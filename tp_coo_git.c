#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rtai_proc_fs.h>
#include <comedilib.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Squelette de programme RTAI et carte ni-6221");
MODULE_AUTHOR("LARBI-LY");


/*
 *	command line parameters
 */


#define ms  1000000

#define microsec 1000

#define CAN 0
#define CNA 1
#define DIO 2
#define CHAN_0 0
#define CHAN_1 1
#define CHAN_2 2
#define CHAN_3 3
#define CHAN_4 4
#define CAN_RANGE 1 // [-5, +5]
#define N 50


static RT_TASK Tache1_Ptr; // Pointeur pour la tache 1
static RT_TASK Tache2_Ptr;


static RT_TASK Handler_Ptr; // Pointeur pour la tache de reprise de main

comedi_t *carte;


void Tache2 (long int x)
{

	lsampl_t S, f, p, a, data;
	
	while(1)
	{

		
		comedi_dio_read(carte, DIO, CHAN_4, 0, &n);	
		comedi_dio_read(carte, DIO, CHAN_0, 0, &S); 
		comedi_dio_read(carte, DIO, CHAN_1, 0, &f);
		comedi_dio_read(carte, DIO, CHAN_2, 0, &p);
		comedi_dio_read(carte, DIO, CHAN_3, 0, &a);
		comedi_data_read(carte, CAN, CHAN_0, 0,AREF_GROUNd, &data);

		rtf_put(fifo1, &n, sizeof(n));
		rtf_put(fifo1, &a, sizeof(a));	
		rtf_put(fifo1, &S, sizeof(S));
		rtf_put(fifo1, &f, sizeof(f));		
		rtf_put(fifo1, &p, sizeof(p));
		rtf_put(fifo1, &data, sizeof(data));	
		rt_sem_signal(&S1);
		
	}
	
}



void Tache1 (long int x)
{
	static int i = 0;
	float coeff = 32767.5;
	double Tab[N] = {0.000000,0.125333,0.248690,0.368125,0.481754,0.587785,0.684547,0.770513,0.844328,0.904827,0.951057,0.982287,0.998027,0.998027,0.982287,0.951057,0.904827,0.844328,0.770513,0.684547,0.587785,0.481754,0.368125,0.248690,0.125333,0.000000,-0.125333,-0.248690,-0.368124,-0.481754,-0.587785,-0.684547,-0.770513,-0.844328,-0.904827,-0.951056,-0.982287,-0.998027,-0.998027,-0.982287,-0.951057,-0.904827,-0.844328,-0.770513,-0.684547,-0.587785,-0.481754,-0.368125,-0.248690,-0.125333 };
  
	lsampl_t TabCNA[N], S, f, p, a, data;
	
	rtf_sem_wait(&S1);
  	rtf_get(fifo1, &n);
  	rtf_get(fifo1, &a);
  	rtf_get(fifo1, &S);
  	rtf_get(fifo1, &f);
  	rtf_get(fifo1, &p);
  	rtf_get(fifo1, &data);
  
  
	if (n) { // signal 1
		if (a) {
			for (i = 0 ; i < N; i++){
				TabCNA[i] = data * coeff * Tab[i] + coeff;
			}
		}
		if (f){   // rt_sleep pour gerer la fréquence
		
			if (S){
			}
			else{

			}
		}
		if (p){

			for (i = 0 ; i < N; i++){
				TabCNA[i] =  coeff * ( Tab[i] + data ) + coeff;
			}

		}
	
		while (1)
		{  
			// Corps de la tâche 1
		
			comedi_data_write(carte, CNA, CHAN_1, 0, AREF_GROUND, TabCNA[i]); // write CNA i

			i = (i+1) % N;	

			rt_task_wait_period();
   		}
	}

	else { // signal 0
		if (a) {
			for (i = 0 ; i < N; i++){
				TabCNA[i] = data * coeff * Tab[i] + coeff;
			}
		}

		if (f){   // rt_sleep pour gerer la fréquence
			
			if (S){
			}
			else{

			}
		}

		if (p){

			for (i = 0 ; i < N; i++){
				TabCNA[i] =  coeff * ( Tab[i] + data ) + coeff;
			}

		}	

 		while (1)
  		{  
    		// Corps de la tâche 1
	
			comedi_data_write(carte, CNA, CHAN_0, 0, AREF_GROUND, TabCNA[i]); // write CNA i

			i = (i+1) % N;	

			rt_task_wait_period();

  	}

}

}




int init_module(void)
{	
	RTIME now, timer_periode;
 	 // Création des tâches
    rt_task_init(&Tache1_Ptr, Tache1, 1, 2000, 2, 0 ,0); // S.S = 2000, PRIO, FPU, PRETASK 
 	rt_task_init(&Tache2_Ptr, Tache2, 1, 2000, 1, 0 ,0); // S.S = 2000, PRIO, FPU, PRETASK 
 	
  	// Initialisation de la carte d'E/S
  	carte = comedi_open("/dev/comedi0");	
  	
	if(carte == NULL)
  	{
    		comedi_perror("Comedi fails to open");
    		return -1;
  	}

	// Configurer le device DIGITAL_INPUT pour recevoir les donnees/signaux
	// et DIGITAL_OUTPUT pour envoyer les donnees/signaux
	// rt_set_oneshot_mode();
	rt_set_periodic_mode();	
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, 0);
	

	// Lancement du timer
	timer_periode = start_rt_timer(nano2count(ms));
	now = rt_get_time();
  	// Lancement des taches
	rt_task_make_periodic(&Tache1_Ptr, now, timer_periode*1); // 1 point toutes les 1 ms T = 50 ms (signal) 
	rt_task_make_periodic(&Tache2_Ptr, now, timer_periode*50); // 1 test tout les 50ms


  return 0;
}


void cleanup_module(void)
{
  stop_rt_timer();
  // Destruction des objets de l'application
  rt_task_delete(&Tache1_Ptr);
  rt_task_delete(&Tache2_Ptr);
  
}