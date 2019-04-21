/**
 *  UNISAL 2019 - Sistemas Operacionais Embarcados - Linux Embarcado
 *  Atividade - Controle de GPIO em threads e leitura de sensor DHT11 via mmap (wiringPi)
 *  Lucas Tamborrino
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>
#include <wiringPi.h>
#include "DHT.h"

#define DHT_PERIOD  15

#define LED1	4
#define LED2	5
#define BTN 	6
#define DHT     29
#define DHT_SENSOR_TYPE  DHT_11

#define DEBOUNCE    0.1 //s

// Variaveis de Referencia para Operacao
bool muda_estado_pisca = false; // inicialmente nao mudamos estado de pisca_led
int led_control = 0;
pthread_mutex_t lock;
pthread_mutex_t stateMutex;


pthread_t thread_id_hb;
pthread_t thread_id_led;
pthread_t thread_id_btn;
pthread_t thread_id_dht;


enum LED_ESTADOS {
    VEL0 = 0,
    VEL1,
    VEL2,
    VEL3,
    VEL4,
    NUM_ESTADOS
};

int VEL_LED[NUM_ESTADOS] = {0, 1000, 200, 100, 1};
int estado_led = VEL0;
volatile bool terminateSignal = 0;

const char* logFreq[] = {"OFF", "1Hz", "5Hz", "10Hz", "ON"};

// Funcoes de Suporte para Thread de Aplicação
void *thread_heart_beat(void *arg);
void *thread_led_ctrl(void *arg);
void *thread_btn_read(void *arg);
void *thread_dht_read(void *arg);


//*********************************************************************************************************
// Função para lidar com sinalização de eventos
void sigintHandler(int sig_num) 
{ 
    syslog(LOG_DEBUG, "\n Terminate \n"); 
    terminateSignal = true;
} 

//*********************************************************************************************************

int main(int argc, char *argv[]) {

    wiringPiSetup() ;
    // Define as direcoes de cada um
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(BTN, INPUT);
  
        // Inicializa threads e mutexes
    if (pthread_mutex_init(&lock, NULL) != 0) {
        fprintf(stderr, "Falha na iniciliazacao do Mutex de mudança de estado\n");
        return 1;
    }
    if (pthread_mutex_init(&stateMutex, NULL) != 0) {
        fprintf(stderr, "Falha na iniciliazacao do Mutex do estado atual\n");
        return 1;
    }
    if (pthread_create(&thread_id_hb, NULL, thread_heart_beat, NULL) != 0) {
        fprintf(stderr, "Falha na criacao da thread de heart beat \n");
        return 1;
    } 

    if (pthread_create(&thread_id_led, NULL, thread_led_ctrl, NULL) != 0) {
        fprintf(stderr, "Falha na criacao da thread de led control 1 \n");
        return 1;
    }
    
    if (pthread_create(&thread_id_btn, NULL, thread_btn_read, NULL) != 0) {
        fprintf(stderr, "Falha na criacao da thread de botao \n");
        return 1;
    }
    
    if (pthread_create(&thread_id_dht, NULL, thread_dht_read, NULL) != 0) {
    fprintf(stderr, "Falha na criacao da thread de leitura DHT \n");
    return 1;
    }

    signal(SIGINT, sigintHandler); 
    
    while(!terminateSignal) 
    {      


    }   
    // join nas threads
    pthread_join(thread_id_hb, NULL);
    pthread_join(thread_id_led, NULL);
    pthread_join(thread_id_btn, NULL);
    pthread_join(thread_id_dht, NULL);


    // Limpa mutex
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&stateMutex);


    return 0;
    
}

//*********************************************************************************************************

void *thread_heart_beat(void *arg) {
  while(!terminateSignal) {
    usleep(0.5 * 1000 * 1000);
    digitalWrite(LED1, HIGH);
    usleep(0.5 * 1000 * 1000);
    digitalWrite(LED1, LOW);
  }

  return NULL;
}

//*********************************************************************************************************

void *thread_led_ctrl(void *arg) {
    
    while(!terminateSignal) {
        usleep(0.100 * 1000 * 1000);
        // Soh precisa da trava/destrava pra leitura de estado pra mudar o comportamento
        pthread_mutex_lock(&lock);
        if (muda_estado_pisca) {

            pthread_mutex_lock(&stateMutex);
            if (estado_led == VEL4) {
                estado_led = VEL0;
            } else {
                estado_led++;
            }
            syslog(LOG_DEBUG, "Muda Estado do Led para F_LED = %s\n", logFreq[estado_led]);
            pthread_mutex_unlock(&stateMutex);
            muda_estado_pisca = false;
        }
        pthread_mutex_unlock(&lock);
        
        switch(estado_led) {
        case VEL0:
            digitalWrite(LED2, LOW);
        break;
        case VEL1:
        case VEL2:
        case VEL3:
            usleep(VEL_LED[estado_led] * 1000);
            digitalWrite(LED2, HIGH);
            usleep(VEL_LED[estado_led] * 1000);
            digitalWrite(LED2, LOW);
        break;
        case VEL4:
            digitalWrite(LED2, HIGH);
        break;
        }

    }
    return NULL;
}


//*********************************************************************************************************

void *thread_btn_read(void *arg) {
    bool old_estado_botao = false;
    bool estado_botao;
    
    
    while(!terminateSignal) {
        usleep(0.100 * 1000 * 1000);
        estado_botao = (digitalRead(BTN) == 0 ? true : false);
        
    

        // checa mudança de estado
        if(estado_botao != old_estado_botao)
        {   
            // Debouce do botão (~50ms)
            usleep(DEBOUNCE * 1000*1000);
            // borda de subida
            if(estado_botao)
            {
                syslog(LOG_DEBUG,"Botão Pressionado!\n");
                pthread_mutex_lock(&lock);
                muda_estado_pisca = true;
                pthread_mutex_unlock(&lock);
                
            }else{ //borda de descida

            }

            old_estado_botao = estado_botao;
        }
    
    }
    return NULL;
}

void *thread_dht_read(void *arg) {
    float temperatura = 0;
    float umidade = 0;
    uint8_t erroCount = 0;

    dht_configure(DHT_SENSOR_TYPE, DHT);

    while(!terminateSignal)
    {
        while(read_dht_data(&temperatura, &umidade) && erroCount < 5)
        {
            erroCount++;
        }
        if(erroCount == 5)
        {
            fprintf(stderr, "PROBLEMA NA LEITURA DO DHT\n");
        }

        erroCount = 0;

        pthread_mutex_lock(&stateMutex);
        syslog(LOG_INFO, "TEMP: %0.0f ºC |UMID: %0.0f\% | LED: %s\n", temperatura, umidade, logFreq[estado_led]);
        pthread_mutex_unlock(&stateMutex);
        
        delay(DHT_PERIOD*1000);
    } 

    return NULL;
}

