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
constexpr int p_5seg     = 5;
 
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
    int id_arch;
    estado_bloque tipo;
    int sig;
};


struct archivo{
int id=0;
string nom;
int tam_kb=0;
int tam_b=0;
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
    int max_arch_b;
    vector<bloque> bloques;
    mt19937 rng;
    
    vector<archivo> archs;
    int total_creados;
    int total_elm;
    int arch_act;
    long long t_acceso;
    long long t_busquedas;
    double suma_frag_ext;
    double max_frag_ext;
    long long muestras_frag;

    chrono::steady_clock::time_point ini;
    bool act=true;

    mutex mtx;
    condition_variable cv;

     int aleatorio(int a, int b) {
        return uniform_int_distribution<int>(a, b)(rng);
    }
    int seg()  {
        return (int)chrono::duration_cast<chrono::seconds>(
                   chrono::steady_clock::now() - ini).count();
    }
    int libres()  {
        int n = 0;
        for (int i=0; i<bloques.size();i++){
            if(bloques[i].id_arch==-1){
                n++;
            }
        }
        return n;
    }
    int mayorHueco()  {
        int mayor = 0, cur = 0;
        for (int i=0;i<bloques.size();i++) {
            if(bloques[i].id_arch==-1){
                cur++;
            }
            if(cur>mayor){
                mayor=cur;
            }
            else{
                cur=0;
            }
        }
        return mayor;
    }
    
    //fragmentación externa

    double fragext(){
        int l=libres();
        if(l==0) return 0;
        return 100*(1-mayorHueco());
    }
    void registrarFrag(){
        double f= fragext();
        suma_frag_ext+=f;
        max_frag_ext=max(max_frag_ext,f);
        ++muestras_frag;
    }
    

    public:
    simDisc(int disc_mb, int bloq_kb, Mecanismo mec, int min_kb, int max_kb, uint32_t sem ){
    this->disc_mb = disc_mb;
    this->bloq_kb = bloq_kb;
    this->bloq_b = bloq_kb*1024;
    this->totalbloq = (disc_mb*1024)/bloq_kb;
    this->mec2 = mec;
    this->min_arch_kb = min_kb;
    this->max_arch_b = max_kb;
    this->bloques.resize(this->totalbloq);
    this->rng.seed(sem);
    }
    void ejecutar(){


    }
};