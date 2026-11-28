#include <algorithm>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
using namespace std;
//variables globales 
constexpr int bytes_p = 4;   
constexpr int creador_arch_seg = 2;   
constexpr int esp_inicial = 30;  
constexpr int p_5seg = 5;
 
enum Mecanismo{
enlazada,
contigua,
indexada

};

enum class estado_bloque{
    libre,
    datos,
    indice

};

struct bloque{
    int id_arch=-1;
    estado_bloque tipo= estado_bloque::libre;
    int sig=-1;
};


struct archivo{
int id=0;
string nom;
long long tam_kb=0;
long long tam_b=0;
vector<int> bloques_dat;
vector<int> bloques_ind;
bool act=false;


};

class simDisc{
    private:
    int disc_mb;
    int bloq_kb;
    int bloq_b;
    int totalbloq;
    Mecanismo mec2;
    int min_arch_kb;
    int max_arch_kb;
    vector<bloque> bloques;
    mt19937 rng;
    
    vector<archivo> archs;
    int total_creados=0;
    int total_elm=0;
    int arch_act=0;
    long long t_acceso=0;
    long long t_busquedas=0;
    double suma_frag_ext=0.0;
    double max_frag_ext=0.0;
    long long muestras_frag=0;

    chrono::steady_clock::time_point ini;
    bool act=true;
    string mot=" simulación finalizada";

    mutex mtx;
    condition_variable cv;

     int aleatorio(int a, int b) {
        return uniform_int_distribution<int>(a, b)(rng);
    }
    int seg()  const{
        return (int)chrono::duration_cast<chrono::seconds>(
                   chrono::steady_clock::now() - ini).count();
    }
    int libres() const {
        int n = 0;
        for (int i=0; i<(int)bloques.size();i++){
            if(bloques[i].id_arch==-1){
                n++;
            }
        }
        return n;
    }
    int mayorHueco() const {
        int mayor = 0, cur = 0;
        for (int i=0;i<(int)bloques.size();i++) {
            if(bloques[i].id_arch==-1){
                cur++;
                
                if(cur>mayor){
                mayor=cur;
            }
            }
    
            else{
                cur=0;
            }
        }
        return mayor;
    }
    
    //fragmentación externa

    double fragext() const{
        int l=libres();
        if(l==0) return 0;
        return 100.0*(l-mayorHueco())/l;
    }
    void registrarFrag(){
        double f= fragext();
        suma_frag_ext+=f;
        max_frag_ext=max(max_frag_ext,f);
        ++muestras_frag;
    }
    
    void mapa(){
        if(totalbloq >150) return; 
        cout<< " mapa: ";
        for(int i=0; i< (int)bloques.size(); i++){
            if(bloques[i].id_arch <0) cout <<".";
            else{
                char base = (bloques[i].tipo == estado_bloque::indice) ? 'a' : 'A';
                cout << (char)(base + (bloques[i].id_arch % 26));
            }

        }
        cout<<endl;
    }
    string nombmec() const {
          return mec2 == contigua ? "Contigua" : mec2 == enlazada ? "Enlazada" : "Indexada";
    }
    void detener( const string &m){
        mot=m;
        cout << "se detuvo la simulación"<< mot<<endl;
        act=false;
        cv.notify_all();

    }
    bool crearArch(){

        long long tam_kb= aleatorio(min_arch_kb,max_arch_kb);
        long long tam_b= tam_kb*1024;
        long long dato, ind=0;
        
        if (mec2 == enlazada){

            long long util=bloq_b-bytes_p;
            dato=(tam_b+util-1)/util;
            
        }
        else {
            dato=(tam_b+bloq_b-1)/bloq_b;
        }
     
    if(dato<1)dato=1;
    if(mec2==indexada){
        long long ptr=bloq_b/bytes_p;
        ind=(dato+ptr-1)/ptr;
    }
    long long n_tot= dato+ ind;
    int lib= libres();
    if(lib<n_tot){
        detener("espacio insuficiente: se requieren" + to_string(n_tot)+ "bloques de "+ to_string(tam_kb)+ "KB y solo hay " + to_string(lib)+ "Libres.");
        return false;

    }

    int inicio_c=-1;
    if(mec2== contigua){
    int segui=0, des =0;
        for (int i =0; i<totalbloq; i++ ){
            if(bloques[i].id_arch==-1){
                if(segui == 0) des=i;
                if(++segui ==n_tot){
                    inicio_c=des; 
                    break;
                }
            
            }
            else{
            segui=0;

            }
        }
        if(inicio_c < 0){

            detener("Fragmentacion externa impide asignacion contigua: se requieren " + to_string(n_tot) + " bloques seguidos (" + to_string(tam_kb) + 
            " KB y el mayor hueco es de " + to_string(mayorHueco()) + " bloques.");

            return false;


        }
        
    }
    archivo a;
    a.id= total_creados;
    a.nom="archivo "+ to_string(a.id+1)+".dat";
    a.tam_b=tam_b;
    a.tam_kb=tam_kb;
    a.act=true;
    
    if(mec2 == contigua){

         for(int i=0; i<dato; i++)
         {
            bloques[i+inicio_c]= {a.id,estado_bloque::datos,-1};
            a.bloques_dat.push_back(inicio_c+i);
         }
    }
    else if (mec2== enlazada){

          int ant = -1;
            for (int i = 0; i < totalbloq && (long long)a.bloques_dat.size() < dato; i++) {
                if (bloques[i].id_arch != -1) continue;

                bloques[i] = {a.id, estado_bloque::datos, -1};
                if (ant >= 0) bloques[ant].sig = i;
                ant = i;
                a.bloques_dat.push_back(i);
            }



    }
    else{
            for (int i = 0; i < totalbloq && ( (long long)a.bloques_ind.size() < ind || (long long)a.bloques_dat.size() < dato); i++) {

                if (bloques[i].id_arch != -1) continue;

                if ((long long)a.bloques_ind.size() < ind) {
                    bloques[i] = {a.id, estado_bloque::indice, -1};
                    a.bloques_ind.push_back(i);
                } 
                
                else {
                    bloques[i] = {a.id, estado_bloque::datos, -1};
                    a.bloques_dat.push_back(i);
                }
            }


    }
    archs.push_back(a);
    total_creados++;
    arch_act++;
    registrarFrag();

    cout<< "t= "<<setw(3)<<seg()<<" crea "<< left<< setw(15)<< a.nom<< "tam"<< right << setw(5) << a.tam_kb << "KB"<<" Bloques "<< setw(4)<< n_tot<< endl;

    if(mec2== contigua)   cout << "Rango " << a.bloques_dat.front() << " " << a.bloques_dat.back() << endl;

    else if (mec2==enlazada) cout << "Inicio "<< a.bloques_dat.front()<< " -> fin " << a.bloques_dat.back()<<endl;
    else {
        
        cout << "Indice " << a.bloques_ind.front() << " del " << a.bloques_ind.size() << " blq, datos: " << a.bloques_dat.size() << " blq "<<endl;
                
                
                }
        cout << " Libres: " << libres() << "  Frag.ext: " << fixed << setprecision(2) << fragext() << endl;
        mapa();

    return true;


    }

    //acceso y simulación

    int elgact(){
        vector<int> v;
        for(int i=0; i< (int)archs.size();i++){
            if(archs[i].act) v.push_back(i);

        }
        if(v.empty()) return -1;
            return v[aleatorio(0,(int)v.size()-1)];
    }
    void simularaccs(){
        int indx=elgact();
        if(indx<0){
            cout<< "t= "<< setw(3)<< seg()<< " s acceso: no hay archivos activos"<<endl;
            return;

        }
        archivo &a= archs[indx];
        long long pos= uniform_int_distribution<long long>(0,a.tam_b-1)(rng);
        long long blog, off;
        int fis, accs;
        if(mec2== contigua){

            blog = pos / bloq_b; 
            off = pos % bloq_b;
            fis = a.bloques_dat[blog];
            accs= 1;

        }
        else if( mec2== enlazada){
            long long utl= bloq_b-bytes_p;
            blog=pos/utl;
            off = pos% utl;
            int cur= a.bloques_dat[0];
            accs=1;
            for(int i =0; i<blog; i++){
                cur=bloques[cur].sig;
                accs++;

            }
            fis=cur;
        }
        else{

            long long ptrs=bloq_b/bytes_p;
            blog=pos/bloq_b;
            off=pos%bloq_b;
            fis=a.bloques_dat[blog];
            accs= (blog/ptrs)+2;

        }
        t_acceso+=accs;
        t_busquedas++;
        registrarFrag();
           cout << " t=" << setw(3) << seg() << "s, acceso " << left << setw(15) << a.nom
             << " Pos logica: Byte " << right << setw(8) << pos  << " blq log=" << blog << ", off=" << off 
             << "B " << " -> Fisico " << fis<< " Accesos: " << accs<< " Frag ext: " << fixed << setprecision(2) << fragext() << endl;

    }

    void elarch(){
    int indx=elgact();
    if(indx<0){
        cout<< "t "<< setw(3)<< seg()<< "s, eliminar si no hay archivos activos"<<endl; 
        return;
    }
    archivo &a = archs[indx];
    int lib=0;
    for (int b : a.bloques_dat) { 
        bloques[b] = bloque{}; 
        ++lib; }

    for (int i: a.bloques_ind){
            bloques[i]=bloque{};
            lib++;
        }

        a.bloques_dat.clear();
        a.bloques_ind.clear();
        a.act=false;
        arch_act--;
        ++total_elm;
        registrarFrag();

        cout << "t: "<< setw(3)<<seg()<< "s, eliminar "<<left<< setw(15)<<a.nom<< " | liberados: "<< right << setw(4) << lib
                << " | libres "<< setw(5)<< libres()<< " | frag ext: "<< fragext()<< endl; ;
        mapa();
    }
    void hilosc(){
        auto prox= ini+chrono::seconds(creador_arch_seg);
        unique_lock<mutex> lk(mtx);
        while(act){
            if(cv.wait_until(lk,prox,[this]{return !act;})) break;
            if(!crearArch())return;
            prox+= chrono:: seconds(creador_arch_seg);
        }
    }
    void Hevent(){
        auto prox = ini + chrono::seconds(esp_inicial);
        unique_lock<mutex> lk(mtx);
        while(act){
              if (cv.wait_until(lk, prox, [this] { return !act; })) break;
            
            simularaccs();
            elarch();
            prox += chrono::seconds(p_5seg);

        }
    }
    void est()const{
        int lib= libres();
        int ocup=totalbloq-lib;
        double prom_frag= muestras_frag ? suma_frag_ext / muestras_frag: 0.0;
        double prom_acc= t_busquedas ? (double)t_acceso/ t_busquedas: 0.0;
        
        cout << "FIN DE LA SIMULACION : "<<endl;
        cout<< "El motivo: "<< mot<< " El mecanismo: "<< nombmec()<<endl;
        cout<< "Duración total: "<<seg() << endl;

        cout << "UTILIZACION DEL DISCO:" <<endl;
        cout << "capacidad: "<< disc_mb<< " MB con un total de bloques"<< totalbloq << "bloques de "<< bloq_kb<< "kb"<<endl;
        cout<< "bloques ocupados: "<< ocup<< " / "<< totalbloq<< "con un porcentaje del "<< 100.0 * ocup / totalbloq<< endl;
        cout<< "Bloques libres: "<<lib<< " y "<<(long long)lib * bloq_kb<<endl;

        cout<< "GESTION DE ARCHIVOS: "<<endl;
        cout << "Creados: " << total_creados << ", " << "Eliminados: " << total_elm << ", " << "Activos: " << arch_act<<endl;
            
        cout << "FRAGMENTACION EXTERNA Y ACCESOS" << endl;
        cout<<"Frag ext final: " << fragext() << "% mayor hueco = " << mayorHueco() << " blqs "<< endl;
        cout<< "Frag ext prom: " << prom_frag << "%" << ", Frag.ext maxima: " << max_frag_ext << ", "
             << "Accesos simulados: " << t_busquedas<< ", Prom. accesos: " << prom_acc << " accesos/busqueda"<< endl ;

        cout << "ARCHIVOS ACTIVOS EN DISCO:  "<<endl;

        int n=0;
        for( const auto &a: archs){
            if(!a.act) continue;
             cout << " - " << left << setw(15) << a.nom << " " << right << setw(5) << a.tam_kb << " KB: ";
            
             if(mec2== contigua){
                cout<<a.bloques_dat.front()<<" "<<a.bloques_dat.back()<<" ";
             }

             else if(mec2==enlazada){
                int lim= min((int)a.bloques_dat.size(),6);

                for(int i=0; i<lim;i++) cout << (i ? "->" : "") << a.bloques_dat[i];
                    if((int)a.bloques_dat.size() >6) cout << " -> "<< a.bloques_dat.back();
                  
             }
             else {
                cout << "Indx:";
                for (size_t i = 0; i < a.bloques_ind.size(); ++i) cout << (i ? "," : "") << a.bloques_ind[i];
                cout << " -> Datos " << a.bloques_dat.size() << " blq: " << a.bloques_dat.front() << " , " << a.bloques_dat.back();
            }
            n++;
            cout<<endl;
        }
         if (n == 0) cout << " Sin archivos activos "<<endl;
        

    }


// setw es utilizado para alinear columnas 
    public:
    simDisc(int disc_mb, int bloq_kb, Mecanismo mec, int min_kb, int max_kb, uint32_t sem ){
    this->disc_mb = disc_mb;
    this->bloq_kb = bloq_kb;
    this->bloq_b = bloq_kb*1024;
    this->totalbloq = (disc_mb*1024)/bloq_kb;
    this->mec2 = mec;
    this->min_arch_kb = min_kb;
    this->max_arch_kb = max_kb;
    this->bloques.resize(this->totalbloq);
    this->rng.seed(sem);
    }
    void ejecutar(){
       
        cout << " Disco: " << disc_mb << " MB (" << totalbloq << " bloques de " << bloq_kb
     << " KB), Mecanismo: " << nombmec() << ", Rango de archivos: " << min_arch_kb
     << " - " << max_arch_kb << " KB, Eventos: crear c/" << creador_arch_seg
     << "s, acceder + eliminar c/" << p_5seg << "s desde t=" << esp_inicial << "s" << endl;

        ini = chrono::steady_clock::now();
        thread t1(&simDisc::hilosc, this);
        thread t2(&simDisc::Hevent, this);
        t1.join();
        t2.join();
        est();

    }
};
int main(){

     int disco_mb = 0, bloque_kb = 0, min_kb = 0, max_kb = 0;
     string mec_;
     Mecanismo mec;

     cout << "--- Simulador de asignacion de archivos ---"<<endl;

      cout << "Tamano del disco (MB): ";
      cin>> disco_mb;
      if(disco_mb<=0 || disco_mb> 2000000){
         cerr<< "Error de tamano invalido "<<endl;
        return 1;
      }

      cout<< "Tamano de bloque (KB): ";
      cin>> bloque_kb;
      if(bloque_kb<=0 || bloque_kb > disco_mb * 1024 ){
        cerr<< "Error de tamano invalido "<<endl;
        return 1;
      }
      if(bloque_kb*1024 <= bytes_p){
        cerr<< "Error el bloque deve ser mayor a  "<<bytes_p<<endl;
        return 1;
      }
       cout << "Mecanismo (contigua / enlazada / indexada): ";
        cin >> mec_;
       transform(mec_.begin(), mec_.end(), mec_.begin(), [](unsigned char c) { return tolower(c); });
    
    if (mec_== "contigua") mec = contigua;
    else if (mec_== "enlazada") mec = enlazada;
    else if (mec_== "indexada") mec = indexada;
    else{
        cerr<< "Error de mecanismo invalido"<<endl;
        return 1;
    }

    cout<< "Tamano minimo de archivo (KB): ";
    cin>> min_kb;
     if(min_kb<= 0 || min_kb > disco_mb *1024) {
     cerr << "Error de tamano minimo invalido"<< endl;
     return 1;
 }
 cout<< "Tamano maximo de archivo (KB): ";
 cin>> max_kb;
 if(max_kb < min_kb || max_kb > disco_mb * 1024) {
     cerr << "Error el tamano maximo invalido"<< endl;
     return 1;
 }

    simDisc sim(disco_mb, bloque_kb, mec, min_kb, max_kb, random_device{}());
    sim.ejecutar();

    return 0;
}