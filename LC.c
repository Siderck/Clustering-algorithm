#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define DIM 20

//ESTA FUNCION CALCULA LA DISTANCIA DESDE UN PUNTO A OTRO Y DEVUELVE 2 VALORES: POSICION Y DISTANCIA MEDIANTE EL USO DE PUNTEROS
void distancia_2puntos(float** centro, float** BD, int posicion, int numero_centro, int* posicion_calculada, float* distancia_calculada)
{
    float aux = 0;
    float distancia = 0;
    for (int j = 0; j < DIM; j++)
    {
        if (BD[posicion][j] == -100)
        {
            distancia = 10000000;
        } 
        else
        {
            distancia = pow((BD[posicion][j] - centro[numero_centro][j]), 2);
            aux = distancia + aux;
        }
    }
    aux = sqrtf(aux);
    *posicion_calculada = posicion;
    *distancia_calculada = aux;
}

//Esta funcion calcula el radio de un cluster
void calcularRadio(float** centro, float** BD, float** distancias, int numero_centro, int posicion, int cantidad_centros){
    float radio;
    for (int i = 0; i < DIM; i++)
    {
        radio = pow((BD[posicion][i] - centro[numero_centro][i]), 2);
        printf("\nRadio: %.3f\n", radio);
    }
}

int main (int argc, char **argv)
{
	int nproc; /* Número de procesos */
	int yo; //Nodo
	int i, j, z, flag=0, cluster_elem_size, cantidad_centros, N_DATOS, K;
    int iteraciones = 0;
	MPI_Status status;
	MPI_Request req;
    float **BD;
    float **centros;
    float **distancias;
    float **radios;
    float **cluster_elementos;
    int **indice;
    float **indice_distancia;
    float **distancias_centro;
    float **distancias_sumatoria;
    int posicion_calculada;
    float distancia_calculada, elementoActual, elementoActual2;
    float* indice_distancia23 = malloc(2*sizeof(float));

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &yo);

    //Lectura datos desde test.txt
    if (yo == 0)
    {
        scanf("%d", &N_DATOS);
        scanf("%d", &K);
        
        BD = (float **)malloc(sizeof(float *)*N_DATOS);
        for ( i = 0; i < N_DATOS; i++)
        {
            BD[i] = (float *)malloc(sizeof(float)*DIM);
        }
    
        for ( i = 0; i < N_DATOS; i++)
        {
            for ( j = 0; j < DIM; j++)
            {
                scanf("%f", &(BD[i][j]));
            }
        }

        //Creación Matriz para almacenar centros de cluster y asignación del primer centro: C0
        cantidad_centros = N_DATOS/(K+1);
        centros = (float **)malloc(sizeof(float *)*cantidad_centros);
        for ( i = 0; i < cantidad_centros; i++)
        {
            centros[i] = (float *)malloc(sizeof(float)*DIM);
        }

        for ( i = 0; i < cantidad_centros; i++)
        {
            for ( j = 0; j < DIM; j++)
            {
                centros[i][j] = BD[0][j]; //Asigna el primer vector de BD como CENTRO 0
                BD[0][j] = -100; //El Primer vector fue asignado, por lo que pasa a ser -100 para marcarlo como utilizado
            }
        }
        
        //Creación Matriz para ir guardando las distancias desde el Centro actual hacia los demas vectores y poder ir calculando los k mas cercanos.
        distancias = (float **)malloc(sizeof(float *)*2);
        for ( i = 0; i < cantidad_centros; i++)
        {
            distancias[i] = (float *)malloc(sizeof(float)*(N_DATOS));
        }

        //Creación Matriz para ir guardando los indices de las distancias
        indice = (int **)malloc(sizeof(int *)*cantidad_centros);
        for ( i = 0; i < cantidad_centros; i++)
        {
            indice[i] = (int *)malloc(sizeof(int)*N_DATOS);
        }

        //Creación Matriz para ir guardando en una dimensión los indices y en la otra dimensión las distancias
        indice_distancia = (float **)malloc(sizeof(float *)*N_DATOS);
        for ( i = 0; i < N_DATOS; i++)
        {
            indice_distancia[i] = (float *)malloc(sizeof(float)*N_DATOS);
        }

        //Creación matriz para almacenar radios de cluster
        radios = (float **)malloc(sizeof(float *)*cantidad_centros);

        cluster_elem_size = cantidad_centros * K;
        //Creación matriz para almacenar elementos de cluster
        cluster_elementos = (float **)malloc(sizeof(float *)*N_DATOS);
        for ( i = 0; i < N_DATOS; i++)
        {
            cluster_elementos[i] = (float *)malloc(sizeof(float)*DIM);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD); //ESTA FUNCION ESPERA QUE TODOS LOS NODOS LLEGUEN HASTA ESTE PUNTO
    
    //Envio datos leidos desde test.txt por yo == 0, hacia los demas nodos
    if (yo == 0)
    {
        for ( i = 1; i < nproc; i++)
        {
            MPI_Send(&N_DATOS, 1, MPI_INT, i, 10000, MPI_COMM_WORLD); //Enviando el entero N_DATOS desde el nodo 0 a los demás
            MPI_Send(&K, 1, MPI_INT, i, 10000, MPI_COMM_WORLD); //Enviando el entero K desde el nodo 0 a los demás
            MPI_Send(&cantidad_centros, 1, MPI_INT, i, 10000, MPI_COMM_WORLD); //Enviando el entero cantidad_centros desde el nodo 0 a los demás
            
            for ( j = 0; j < N_DATOS; j++)
            {
                MPI_Send(BD[j], DIM, MPI_FLOAT, i, 10000, MPI_COMM_WORLD);  //Enviando el array BD desde nodo 0 a los demás
            }
            
            for ( z = 0; z < cantidad_centros; z++)
            {
                MPI_Send(centros[z], DIM, MPI_FLOAT, i, 10000, MPI_COMM_WORLD);  //Enviando el array centro desde nodo 0 a los demás
            }
            
            for ( z = 0; z < cantidad_centros; z++)
            {
                MPI_Send(distancias[z], N_DATOS, MPI_FLOAT, i, 10000, MPI_COMM_WORLD);  //Enviando el array distancias desde nodo 0 a los demás
            }

            for ( z = 0; z < cantidad_centros; z++)
            {
                MPI_Send(indice[z], N_DATOS, MPI_INT, i, 10000, MPI_COMM_WORLD);  //Enviando el array indice desde nodo 0 a los demás
            }

            for ( j = 0; j < N_DATOS; j++)
            {
                MPI_Send(indice_distancia[j], N_DATOS, MPI_FLOAT, i, 10000, MPI_COMM_WORLD);  //Enviando el indice_distancia BD desde nodo 0 a los demás
            }

            for ( j = 0; j < N_DATOS; j++)
            {
                MPI_Send(cluster_elementos[j], DIM, MPI_FLOAT, i, 10000, MPI_COMM_WORLD);  //Enviando el cluster_elementos BD desde nodo 0 a los demás
            }
            

        }
    }
    
    
    else  { 
        //LOS OTROS NODOS RECIBEN LOS DATOS DE TEST.TXT
        
        MPI_Recv(&N_DATOS, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //Recibiendo el entero N_DATOS en todos los nodos
        MPI_Recv(&K, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //Recibiendo el entero K en todos los nodos
        MPI_Recv(&cantidad_centros, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //Recibiendo el entero cantidad_centros (el cual define la cantidad de posibles centros)
        
        BD = (float **)malloc(sizeof(float *)*N_DATOS);
        for ( i = 0; i < N_DATOS; i++)
        {
            BD[i] = (float *)malloc(sizeof(float)*DIM);
        }

        centros = (float **)malloc(sizeof(float *)*cantidad_centros);
        for ( i = 0; i < cantidad_centros; i++)
        {
            centros[i] = (float *)malloc(sizeof(float)*DIM);
        }

        distancias = (float **)malloc(sizeof(float *)*2);
        for ( i = 0; i < cantidad_centros; i++)
        {
            distancias[i] = (float *)malloc(sizeof(float)*(N_DATOS));
        }

        indice = (int **)malloc(sizeof(int *)*cantidad_centros);
        for ( i = 0; i < cantidad_centros; i++)
        {
            indice[i] = (int *)malloc(sizeof(int)*N_DATOS);
        }

        indice_distancia = (float **)malloc(sizeof(float *)*N_DATOS);
        for ( i = 0; i < N_DATOS; i++)
        {
            indice_distancia[i] = (float *)malloc(sizeof(float)*N_DATOS);
        }

        cluster_elementos = (float **)malloc(sizeof(float *)*N_DATOS);
        for ( i = 0; i < N_DATOS; i++)
        {
            cluster_elementos[i] = (float *)malloc(sizeof(float)*DIM);
        }

        for ( j = 0; j < N_DATOS; j++)
        {
            MPI_Recv(BD[j], DIM, MPI_FLOAT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //Recibiendo el array BD en todos los nodos
        }

        for ( z = 0; z < cantidad_centros; z++)
        {
            MPI_Recv(centros[z], DIM, MPI_FLOAT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //Recibiendo el array centros en todos los nodos
        }

        for ( z = 0; z < cantidad_centros; z++)
        {
            MPI_Recv(distancias[z], N_DATOS, MPI_FLOAT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //Recibiendo el array distancias en todos los nodos
        }

        for ( z = 0; z < cantidad_centros; z++)
        {
            MPI_Recv(indice[z], N_DATOS, MPI_FLOAT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //Recibiendo el array indice en todos los nodos
        }

        for ( j = 0; j < N_DATOS; j++)
        {
            MPI_Recv(indice_distancia[j], N_DATOS, MPI_FLOAT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);  //Enviando el array indice_distancia desde nodo 0 a los demás
        }

        for ( j = 0; j < N_DATOS; j++)
        {
            MPI_Recv(cluster_elementos[j], DIM, MPI_FLOAT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);  //Enviando el array cluster_elementos desde nodo 0 a los demás
        }

    }

    //Ciclo iterativo del algoritmo para crear clusters
    while (1)
    {
        int contador = 0;
        // Recorre toda la BD, por cada elemento que sea == a -100 suma +1 a CONTADOR
        for ( i = 0; i < N_DATOS; i++)
        {
            for ( j = 0; j < DIM; j++)
            {
                if (BD[i][j] == -100)
                {
                    contador++;
                }
            }
        }
           
        // Si todos los elementos de la BD son -100, se detiene el algoritmo porque no hay mas datos que almacenar en un cluster
        if (contador == N_DATOS * DIM)
        {
            break;
        } else {
            contador = 0;
        }
        
        if (iteraciones == 0 || iteraciones == 1)
        {     
            for ( i = 0; i < N_DATOS; i++)
            {
                if ( i % nproc == yo) // Se utilizan todos los nodos para realizar los calculos de distancia
                {
                    distancia_2puntos(centros, BD, i, iteraciones, &posicion_calculada, &distancia_calculada); //iteraciones = a cual centro corresponde
                    indice_distancia23[0] = posicion_calculada;
                    indice_distancia23[1] = distancia_calculada;
                    MPI_Send(indice_distancia23, 2, MPI_FLOAT, 0, 10005, MPI_COMM_WORLD);  //Enviando el array indice_distancia23 desde nodo i a los demás
                }
            }

            MPI_Barrier(MPI_COMM_WORLD); //ESTA FUNCION ESPERA QUE TODOS LOS NODOS LLEGUEN HASTA ESTE PUNTO

            if ( yo == 0) // yo == 0 Recibir todos los datos
            {
                for ( j = 0; j < N_DATOS; j++)
                {
                    for ( i = 0; i < 1; i++)
                    {
                        float* indice_distancia23 = malloc(2*sizeof(float));
                        MPI_Recv(indice_distancia23, 2, MPI_FLOAT, MPI_ANY_SOURCE, 10005, MPI_COMM_WORLD, &status);  //Enviando el array indice_distancia23 desde nodo i a los demás 
                        int convertir_indice = indice_distancia23[0];
                        indice[i][j] = convertir_indice;
                        distancias[i][j] = indice_distancia23[1];
                    } 
                }
                
                int izquierda = 0;
                int derecha = N_DATOS;
                int tamano_cluster = K+1;
                float distancias_ordenadas[N_DATOS];
                int indices_ordenados[N_DATOS];
                int z = 0;
                int i;
                int j;

                while (derecha - izquierda >= N_DATOS)
                {
                    if ((distancias[izquierda]) > (distancias[derecha])) {
                        izquierda++;
                    }
                    else {
                        derecha--;
                    }
                }

                //Los indices se ordenan a la vez que se ordenan las distancias tambien
                while (izquierda <= derecha)
                {
                    distancias_ordenadas[z] = distancias[0][izquierda-1];
                    indices_ordenados[z] = indice[0][izquierda-1];
                    z++;
                    izquierda++;
                }

                //Ordenando arreglo de distancias K de manera ascendente
                float a;
                int b;
                for (i = 0; i < N_DATOS; ++i) 
                {
                    for (j = i + 1; j < N_DATOS; ++j)
                    {
                        if (distancias_ordenadas[i] > distancias_ordenadas[j]) 
                        {
                            a =  distancias_ordenadas[i];
                            b = indices_ordenados[i];
                            distancias_ordenadas[i] = distancias_ordenadas[j];
                            indices_ordenados[i] = indices_ordenados[j];
                            distancias_ordenadas[j] = a;
                            indices_ordenados[j] = b;
                        }
                    }
                }

                // SOLO SI LA ITERACION == 0 ASIGNA EL CENTRO 2
                if (iteraciones == 0)
                {
                    //Asignacion Centro 2
                    for ( j = 0; j < DIM; j++)
                    {
                        centros[1][j] = BD[indices_ordenados[N_DATOS-1]][j]; //Asigna el elemento de BD mas lejano al CENTRO 0 como CENTRO 1
                        BD[indices_ordenados[N_DATOS-1]][j] = -100; //El segundo centro fue asignado, por lo que pasa a ser -100 en BD para marcarlo
                    }
                }
                
                // Se asignan los K elementos al Cluster 0
                if (iteraciones == 0)
                {
                    for ( i = 1; i < K+1; i++)
                    {
                        for ( j = 0; j < DIM; j++)
                        {
                            if (BD[indices_ordenados[i]][j] != -100)
                            {
                                cluster_elementos[i][j] = BD[indices_ordenados[i]][j];
                                BD[indices_ordenados[i]][j] = -100;
                            }
                        }
                    }
                }

                // Se asignan los K elementos al Cluster 1
                if (iteraciones == 1)
                {
                    int contarK = 0; 

                    for ( i = 0; i < N_DATOS; i++)
                    {
                        for ( j = 0; j < DIM; j++)
                        {
                            if (distancias_ordenadas[i] != 0.000)
                            {
                                if (BD[indices_ordenados[i]][j] != -100 && contarK != K*DIM)
                                {
                                    cluster_elementos[i][j] = BD[indices_ordenados[i]][j];                                
                                    BD[ indices_ordenados[i] ][j] = -100;
                                    contarK++;
                                }
                            }
                        }
                    }
                }
                     
                if (iteraciones == 0)
                {
                    printf("\n Coordenadas Centro Cluster %d: \n", iteraciones+1);
                    for ( i = 0; i < 1; i++)
                    {
                        for ( j = 0; j < DIM; j++)
                        {
                            printf(" %.3f", centros[i][j]);
                        }  
                    }

                    printf("\n\n Coordenadas de cada elemento del Cluster %d: \n", iteraciones+1);
                    for ( i = 1; i < K+1; i++)
                    {
                        for ( j = 0; j < DIM; j++)
                        {
                            if (cluster_elementos[i][j] == -100.000)
                            {
                                i++;
                            }
                            printf(" %.3f", cluster_elementos[i][j]);
                        }
                        printf("\n");
                    }
                }

                if (iteraciones == 1)
                {
                    printf("\n");
                    printf("\n Coordenadas Centro Cluster 2: \n");
                    for ( i = 1; i < 2; i++)
                    {
                        for ( j = 0; j < DIM; j++)
                        {
                            printf(" %.3f", centros[i][j]);
                        }
                        
                        
                    }
  
                    printf("\n\n Coordenadas de cada elemento del Cluster 2: \n");
                    int contarK = 0;

                    for ( i = K+2; i < N_DATOS; i++)
                    {
                        for ( j = 0; j < DIM; j++)
                        {
                            if (cluster_elementos[i][j] == -100.000)
                            {
                                i++;
                            }
                            if (contarK != K*DIM)
                            {
                                printf(" %.3f", cluster_elementos[i][j]);
                                contarK++;
                            } 
                        }
                        if (contarK != K*DIM)
                        {
                            printf("\n");
                        }
                    }
                    printf("\n");
                }

                for ( i = 1; i < nproc; i++)
                {
                    for ( j = 0; j < cantidad_centros; j++)
                    {
                        MPI_Send(centros[j], DIM, MPI_FLOAT, i, 10010, MPI_COMM_WORLD);  //Enviando el array centros desde nodo 0 a los demás
                    } 
                }

                for ( i = 1; i < nproc; i++)
                {
                    for ( j = 0; j < N_DATOS; j++)
                    {
                        MPI_Send(BD[j], DIM, MPI_FLOAT, i, 10020, MPI_COMM_WORLD);  //Enviando el array BD desde nodo 0 a los demás
                    } 
                }
            } 
            else { // Si no estamos en el nodo 0, entonces recibimos todas las actualizaciones de los array BD y centros en todos los otros nodos
            
                for ( j = 0; j < cantidad_centros; j++)
                {
                    MPI_Recv(centros[j], DIM, MPI_FLOAT, MPI_ANY_SOURCE, 10010, MPI_COMM_WORLD, &status); //Recibiendo el array BD en todos los nodos
                }

                for ( j = 0; j < N_DATOS; j++)
                {
                    MPI_Recv(BD[j], DIM, MPI_FLOAT, MPI_ANY_SOURCE, 10020, MPI_COMM_WORLD, &status); //Recibiendo el array BD en todos los nodos
                }
            }            
        }
        MPI_Barrier(MPI_COMM_WORLD); //ESTA FUNCION ESPERA QUE TODOS LOS NODOS LLEGUEN HASTA ESTE PUNTO

	

        //PARTE 4) Trabajo
        if (iteraciones > 1)
        {
            if (yo == 0)
            {
                //Arreglo que contiene las distancias desde los centros a los puntos
                distancias_centro = (float **)malloc(sizeof(float *)*iteraciones);
                for ( i = 0; i < iteraciones; i++)
                {
                    distancias_centro[i] = (float *)malloc(sizeof(float)*N_DATOS);
                }
                
                //Arreglo que va guardando la sumatoria de las distancias desde Ci hacia e
                distancias_sumatoria = (float **)malloc(sizeof(float *)*1);
                for ( i = 0; i < 1; i++)
                {
                    distancias_sumatoria[i] = (float *)malloc(sizeof(float)*N_DATOS);
                }

                //Se inicializa el arreglo distancias_sumatoria en 0
                for ( i = 0; i < 1; i++)
                {
                    for ( j = 0; j < N_DATOS; j++)
                    {
                        distancias_sumatoria[i][j] = 0;
                    }
                }

                //Se calcula la distancia desde los centros hacia los vectores
                for ( i = 0; i < iteraciones; i++)
                {
                    for ( j = 0; j < N_DATOS; j++)
                    {
                        distancia_2puntos(centros, BD, j, i, &posicion_calculada, &distancia_calculada);
                        distancias_centro[i][j] = distancia_calculada;
                    }
                }

                float mayor_distancia = distancias_centro[0][0];
                int posicion_distancia_mayor = 0;

                //Se realiza la sumatoria de distancia desde Ci hacia e
                for ( i = 0; i < iteraciones; i++)
                {
                    for ( j = 0; j < N_DATOS; j++)
                    {
                        distancias_sumatoria[0][j] =  distancias_sumatoria[0][j] + distancias_centro[i][j];
                    }    
                }
                
                printf("\n\n");
                //Se busca la mayor de las distancias dentro de la sumatoria
                for ( j = 0; j < N_DATOS; j++)
                {
                    elementoActual = distancias_sumatoria[0][j];
                    if (elementoActual > mayor_distancia) mayor_distancia = elementoActual;
                }
                
                //Se busca la posicion de la mayor de las distancias hacia el centro
                for ( i = 0; i < iteraciones; i++)
                {
                    int aux_posicion_distancia_mayor = 0;
                    
                    for ( j = 0; j < N_DATOS; j++)
                    {
                        if (distancias_sumatoria[0][j] == mayor_distancia)
                        {
                            posicion_distancia_mayor = aux_posicion_distancia_mayor;
                            break;
                        }
                        aux_posicion_distancia_mayor++;
                    }
                }

                //Se asigna el vector que será el nuevo centro a partir de la sumatoria realizada(corregir posicion_distancia_mayor)
                for ( j = 0; j < DIM; j++)
                {
                    centros[iteraciones][j] = BD[posicion_distancia_mayor][j]; //Asigna el el vector mas lejano entre las distancias desde centros hacia elementos como nuevo centro
                    BD[posicion_distancia_mayor][j] = -100; //El nuevo centro es marcado reasignando sus valores a -100
                }
                
                printf("Coordenadas Centro Cluster %d: \n", iteraciones+1);
                for ( i = iteraciones; i <= iteraciones; i++)
                {
                    for ( j = 0; j < DIM; j++)
                    {
                        printf("%.3f  ", centros[i][j]);
                    }                    
                }

                for ( i = 1; i < nproc; i++)
                {
                    for ( j = 0; j < cantidad_centros; j++)
                    {
                        MPI_Send(centros[j], DIM, MPI_FLOAT, i, 20020, MPI_COMM_WORLD);  //Enviando el array centros desde nodo 0 a los demás
                    } 
                }

                for ( i = 1; i < nproc; i++)
                {
                    for ( j = 0; j < N_DATOS; j++)
                    {
                        MPI_Send(BD[j], DIM, MPI_FLOAT, i, 20030, MPI_COMM_WORLD);  //Enviando el array BD desde nodo 0 a los demás
                    } 
                }


            } else { // Si no estamos en el nodo 0, entonces recibimos todas las actualizaciones de BD en todos los otros nodos
            
                for ( j = 0; j < cantidad_centros; j++)
                {
                    MPI_Recv(centros[j], DIM, MPI_FLOAT, MPI_ANY_SOURCE, 20020, MPI_COMM_WORLD, &status); //Recibiendo el array BD en todos los nodos
                }

                for ( j = 0; j < N_DATOS; j++)
                {
                    MPI_Recv(BD[j], DIM, MPI_FLOAT, MPI_ANY_SOURCE, 20030, MPI_COMM_WORLD, &status); //Recibiendo el array BD en todos los nodos
                }
            }
            MPI_Barrier(MPI_COMM_WORLD); //ESPERA QUE EL NUEVO CENTRO SEA ASIGNADO Y SE ACTUALICE EL ARRAY CENTROS y ARRAY BD

            for ( i = 0; i < N_DATOS; i++)
            {          
                if ( i % nproc == yo) // Se utilizan todos los nodos para realizar los calculos de distancia
                {
                    distancia_2puntos(centros, BD, i, iteraciones, &posicion_calculada, &distancia_calculada); //iteraciones = a cual centro corresponde
                    indice_distancia23[0] = posicion_calculada;
                    indice_distancia23[1] = distancia_calculada;
                    MPI_Send(indice_distancia23, 2, MPI_FLOAT, 0, iteraciones, MPI_COMM_WORLD);  //Enviando el array BD desde nodo i a los demás
                }
            }

            MPI_Barrier(MPI_COMM_WORLD); //ESTA FUNCION ESPERA QUE TODOS LOS NODOS LLEGUEN HASTA ESTE PUNTO
            if ( yo == 0) // yo == 0 Recibir todos los datos
            {
                for ( j = 0; j < N_DATOS; j++)
                {
                    for ( i = 0; i < 1; i++)
                    {
                        float* indice_distancia23 = malloc(2*sizeof(float));
                        MPI_Recv(indice_distancia23, 2, MPI_FLOAT, MPI_ANY_SOURCE, iteraciones, MPI_COMM_WORLD, &status);  //Enviando el array BD desde nodo i a los demás 
                        int convertir_indice = indice_distancia23[0];
                        indice[i][j] = convertir_indice;
                        distancias[i][j] = indice_distancia23[1];
                    } 
                }

                int izquierda = 0;
                int derecha = N_DATOS;
                int tamano_cluster = K+1;
                float distancias_ordenadas[N_DATOS];
                int indices_ordenados[N_DATOS];
                int z = 0;
                int i;
                int j;

                while (derecha - izquierda >= N_DATOS)
                {
                    if ((distancias[izquierda]) > (distancias[derecha])) {
                        izquierda++;
                    }
                    else {
                        derecha--;
                    }
                }

                //Los indices se ordenan a la vez que se ordenan las distancias tambien
                while (izquierda <= derecha)
                {
                    distancias_ordenadas[z] = distancias[0][izquierda-1];
                    indices_ordenados[z] = indice[0][izquierda-1];
                    z++;
                    izquierda++;
                }

                //Ordenando arreglo de distancias K de manera ascendente
                float a;
                int b;
                for (i = 0; i < N_DATOS; ++i) 
                {
                    for (j = i + 1; j < N_DATOS; ++j)
                    {
                        if (distancias_ordenadas[i] > distancias_ordenadas[j]) 
                        {
                            a =  distancias_ordenadas[i];
                            b = indices_ordenados[i];
                            distancias_ordenadas[i] = distancias_ordenadas[j];
                            indices_ordenados[i] = indices_ordenados[j];
                            distancias_ordenadas[j] = a;
                            indices_ordenados[j] = b;
                        }
                    }
                }

                int contar_K = 0;
                printf("\n\nCoordenadas de cada elemento del Cluster %d:\n",iteraciones+1);
                
                for ( i = 0; i < N_DATOS; i++)
                {
                    for ( j = 0; j < DIM; j++)
                    {
                        if (distancias_ordenadas[i] != 0.000)
                        {
                            if (BD[indices_ordenados[i]][j] != -100 && contar_K != K*DIM)
                            {
                                cluster_elementos[i][j] = BD[indices_ordenados[i]][j];
                                printf(" %.3f ", cluster_elementos[i][j]);
                                BD[indices_ordenados[i]][j] = -100;
                                contar_K++;
                            }
                        }
                    }
                    if (distancias_ordenadas[i] != 0.000)
                    {
                        if (BD[indices_ordenados[i]][j] != -100 && contar_K != K*DIM)
                        {
                            printf("\n");
                        }
                    }    
                }
                printf("\n");

                for ( i = 1; i < nproc; i++)
                {
                    for ( j = 0; j < cantidad_centros; j++)
                    {
                        MPI_Send(centros[j], DIM, MPI_FLOAT, i, 30000+iteraciones, MPI_COMM_WORLD);  //Enviando el array centros desde nodo 0 a los demás
                    } 
                }

                for ( i = 1; i < nproc; i++)
                {
                    for ( j = 0; j < N_DATOS; j++)
                    {
                        MPI_Send(BD[j], DIM, MPI_FLOAT, i, 50000+iteraciones, MPI_COMM_WORLD);  //Enviando el array BD desde nodo 0 a los demás
                    } 
                }
            
            } else { // Si no estamos en el nodo 0, entonces recibimos todas las actualizaciones de BD en todos los otros nodos

                for ( j = 0; j < cantidad_centros; j++)
                {
                    MPI_Recv(centros[j], DIM, MPI_FLOAT, MPI_ANY_SOURCE, 30000+iteraciones, MPI_COMM_WORLD, &status); //Recibiendo el array centros en todos los nodos
                }

                for ( j = 0; j < N_DATOS; j++)
                {
                    MPI_Recv(BD[j], DIM, MPI_FLOAT, MPI_ANY_SOURCE, 50000+iteraciones, MPI_COMM_WORLD, &status); //Recibiendo el array BD en todos los nodos
                }
            }            
        }
        MPI_Barrier(MPI_COMM_WORLD); //ESTA FUNCION ESPERA QUE TODOS LOS NODOS LLEGUEN HASTA ESTE PUNTO
        iteraciones++;
    } 
    MPI_Finalize();
	return 0;
}
