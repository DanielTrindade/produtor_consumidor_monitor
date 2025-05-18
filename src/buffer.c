#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define TAM_BUFFER 5
#define N_PRODUTORES 2
#define N_CONSUMIDORES 2
#define MAX_ITERACOES 20

int buffer[TAM_BUFFER];
int in = 0, out = 0, count = 0;
int total_produzido = 0;
int produtores_ativos = N_PRODUTORES;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_prod = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_cons = PTHREAD_COND_INITIALIZER;

void* produtor(void* arg) {
    int id = *((int*)arg);
    int valor;
    
    for (int i = 0; i < MAX_ITERACOES; i++) {
        valor = rand() % 100;
        
        pthread_mutex_lock(&lock);
        // Se está com o buffer cheio vai aguardar até ter elementos consumidos
        while (count == TAM_BUFFER) {
            printf("[Produtor %d] Buffer cheio. Aguardando...\n", id);
            pthread_cond_wait(&cond_prod, &lock);
        }
        
        buffer[in] = valor;
        printf("[Produtor %d] Inseriu: %d\n", id, valor);
        
        in = (in + 1) % TAM_BUFFER;
        count++;
        total_produzido++;
        
        pthread_cond_signal(&cond_cons);
        pthread_mutex_unlock(&lock);
        
        usleep(rand() % 500000);
    }
    
    pthread_mutex_lock(&lock);
    produtores_ativos--;
    
    // Quando o último produtor terminar, acorda todos os consumidores
    if (produtores_ativos == 0) {
        printf("[Sistema] Todos os produtores terminaram. Notificando consumidores...\n");
        pthread_cond_broadcast(&cond_cons);
    }
    
    pthread_mutex_unlock(&lock);
    
    return NULL;
}

void* consumidor(void* arg) {
    int id = *((int*)arg);
    int valor;
    int itens_consumidos = 0;
    
    while (itens_consumidos < MAX_ITERACOES) {
        pthread_mutex_lock(&lock);
        
        // Verifica se o buffer está vazio E se ainda há possibilidade de produção
        while (count == 0) {
            // Se não há itens e todos os produtores terminaram, encerra
            if (produtores_ativos == 0 && count == 0) {
                printf("[Consumidor %d] Não há mais produtores ativos e o buffer está vazio. Encerrando...\n", id);
                pthread_mutex_unlock(&lock);
                return NULL;
            }
            
            printf("[Consumidor %d] Buffer vazio. Aguardando...\n", id);
            pthread_cond_wait(&cond_cons, &lock);
            
            // Verifica novamente após o wait
            if (produtores_ativos == 0 && count == 0) {
                printf("[Consumidor %d] Acordou, mas não há mais produtores e o buffer está vazio. Encerrando...\n", id);
                pthread_mutex_unlock(&lock);
                return NULL;
            }
        }
        
        valor = buffer[out];
        printf("[Consumidor %d] Removeu: %d\n", id, valor);
        
        out = (out + 1) % TAM_BUFFER;
        count--;
        itens_consumidos++;
        
        pthread_cond_signal(&cond_prod);
        pthread_mutex_unlock(&lock);
        
        usleep(rand() % 800000);
    }
    
    printf("[Consumidor %d] Completou %d iterações. Encerrando...\n", id, itens_consumidos);
    return NULL;
}

int main() {
    pthread_t produtores[N_PRODUTORES], consumidores[N_CONSUMIDORES];
    int ids_prod[N_PRODUTORES], ids_cons[N_CONSUMIDORES];
    
    srand(time(NULL));

    for (int i = 0; i < N_PRODUTORES; i++) {
        ids_prod[i] = i + 1;
        pthread_create(&produtores[i], NULL, produtor, &ids_prod[i]);
    }

    for (int i = 0; i < N_CONSUMIDORES; i++) {
        ids_cons[i] = i + 1;
        pthread_create(&consumidores[i], NULL, consumidor, &ids_cons[i]);
    }

    for (int i = 0; i < N_PRODUTORES; i++)
        pthread_join(produtores[i], NULL);
    for (int i = 0; i < N_CONSUMIDORES; i++)
        pthread_join(consumidores[i], NULL);
        
    printf("\n[Sistema] Resumo final:\n");
    printf("Total de itens produzidos: %d\n", total_produzido);
    printf("Itens restantes no buffer: %d\n", count);
    
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond_prod);
    pthread_cond_destroy(&cond_cons);

    return 0;
}