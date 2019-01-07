#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

pthread_mutex_t m1;
pthread_mutex_t m2;

volatile bool should_wait = true;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

const size_t NUM_OF_ITER = 1000000;

typedef struct {
    int *r1;
    int *r2;
    //Dependencies
    const int *dep1;
    const int *dep2;
} pthread_args;

void print_results(int *r1, int*r2)
{
    printf("Results:\n");
    printf("\tr1 = %d\n", *r1);
    printf("\tr2 = %d\n", *r2);
    printf("\n");
}

void *t1(void *args)
{
    int *r1 = ((pthread_args*)args)->r1;
    int *r2 = ((pthread_args*)args)->r2;

    for(int i=NUM_OF_ITER; i>0; --i)
    {
        pthread_mutex_lock(&m1);
        ++(*r1);
        pthread_mutex_unlock(&m1);
    }

    for(int i=NUM_OF_ITER; i>0; --i)
    {
        pthread_mutex_lock(&m2);
        ++(*r2);
        pthread_mutex_unlock(&m2);
    }
}

void *t2(void *args)
{
    int *r1 = ((pthread_args*)args)->r1;
    int *r2 = ((pthread_args*)args)->r2;

    for(int i=NUM_OF_ITER; i>0; --i)
    {
        pthread_mutex_lock(&m2);
        ++(*r2);
        pthread_mutex_unlock(&m2);
    }

    for(int i=NUM_OF_ITER; i>0; --i)
    {
        pthread_mutex_lock(&m1);
        ++(*r1);
        pthread_mutex_unlock(&m1);
    }
}

void *t3(void *args)
{
    int *r1 = ((pthread_args*)args)->r1;
    int *r2 = ((pthread_args*)args)->r2;

    for(int i=NUM_OF_ITER; i>0; --i)
    {
        pthread_mutex_lock(&m2);
        ++(*r2);
        pthread_mutex_unlock(&m2);
    }

    for(int i=NUM_OF_ITER; i>0; --i)
    {
        pthread_mutex_lock(&m1);
        ++(*r1);
        pthread_mutex_unlock(&m1);
    }   
}

void *t4(void *args)
{
    pthread_mutex_lock(&m1);
    should_wait = true;

    ((pthread_args*)args)->r1 = (int*)malloc(sizeof(int));
    ((pthread_args*)args)->r2 = (int*)malloc(sizeof(int));
    (*((pthread_args*)args)->r1) = 50 + *((pthread_args*)args)->dep1;
    (*((pthread_args*)args)->r2) = 70 + *((pthread_args*)args)->dep2;

    pthread_cond_signal(&cond_var);
    should_wait = false;
    pthread_mutex_unlock(&m1);
}

void *t5(void *args)
{
    pthread_mutex_lock(&m1);
    
    while(should_wait) pthread_cond_wait(&cond_var, &m1);

    print_results(((pthread_args*)args)->r1, ((pthread_args*)args)->r2);
    free(((pthread_args*)args)->r1);
    free(((pthread_args*)args)->r2);

    pthread_mutex_unlock(&m1);
}

void *t6(void *args)
{
    pthread_mutex_lock(&m2);
    print_results(((pthread_args*)args)->r1, ((pthread_args*)args)->r2);
    pthread_mutex_unlock(&m2);
}

int main()
{
    
    int r1=0;
    int r2=0;

    pthread_args t1args = {&r1, &r2};
    pthread_args t2args = {&r1, &r2};

    {
        pthread_t t1_id;
        pthread_t t2_id;
        
        printf("Start calculations in t1 and t2\n");
        pthread_create(&t1_id, NULL, t1, (void*)(&t1args));
        pthread_create(&t2_id, NULL, t2, (void*)(&t2args));
        pthread_join(t1_id, NULL);
        pthread_join(t2_id, NULL);
        print_results(&r1, &r2);
    }
    
    {
        
        pthread_args t3args = {&r1, &r2};

        pthread_t t3_id;
        
        printf("Start calculations in t3 and t0\n");
        pthread_create(&t3_id, NULL, t3, (void*)(&t3args));

        for(int i=NUM_OF_ITER; i>0; --i)
        {
            pthread_mutex_lock(&m1);
            ++r1;
            pthread_mutex_unlock(&m1);
        }

        for(int i=NUM_OF_ITER; i>0; --i)
        {
            pthread_mutex_lock(&m2);
            ++r2;
            pthread_mutex_unlock(&m2);
        } 

        pthread_join(t3_id, NULL);

        print_results(&r1, &r2);
    }
    
    {
        printf("Start ordered action in parallel threads\n");
        pthread_args t4t5args = {.dep1 = &r1, .dep2 = &r2};
        
        pthread_t t4_id;
        pthread_t t5_id;

        pthread_create(&t4_id, NULL, t4, (void*)(&t4t5args));
        pthread_create(&t5_id, NULL, t5, (void*)(&t4t5args));

        pthread_join(t4_id, NULL);
        pthread_join(t5_id, NULL);
    }

    {
        pthread_t t6_id;
        
        printf("Parallel printing results from t6 and t0\n");
        pthread_create(&t6_id, NULL, t6, (void*)(&t1args));

        pthread_mutex_lock(&m2);
        print_results(&r1, &r2);
        pthread_mutex_unlock(&m2);
        
        pthread_join(t6_id, NULL);
    }

    return 0;
}
