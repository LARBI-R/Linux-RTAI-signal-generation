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
#define PI 3.14159
#define fifo1 1


static RT_TASK Tache1_Ptr; // Pointeur pour la tache 1
static RT_TASK Tache2_Ptr;
static SEM S1;

static RT_TASK Handler_Ptr; // Pointeur pour la tache de reprise de main

comedi_t *carte;

void Tache2 (long int x)
{

	lsampl_t S, f, p, a, data;

	
	while(1)
	{
		// lecture des entrees digitales	
		comedi_dio_read(carte, DIO, CHAN_4, 0, &n);	
		comedi_dio_read(carte, DIO, CHAN_0, 0, &S); 
		comedi_dio_read(carte, DIO, CHAN_1, 0, &f);
		comedi_dio_read(carte, DIO, CHAN_2, 0, &p);
		comedi_dio_read(carte, DIO, CHAN_3, 0, &a);

		// lecture de l'entree analogique du CAN
		comedi_data_read(carte, CAN, CHAN_0, CAN_RANGE, AREF_GROUND, &data);

		// empiler les donnees dans la fifo
		rtf_put(fifo1, &n, sizeof(n));
		rtf_put(fifo1, &a, sizeof(a));
		rtf_put(fifo1, &S, sizeof(S));
		rtf_put(fifo1, &f, sizeof(f));		
		rtf_put(fifo1, &p, sizeof(p));
		rtf_put(fifo1, &data, sizeof(data));	// valeur du CAN 
		
		// lacher la semaphore V
		rt_sem_signal(&S1);

		// arret de la tache
		rt_task_suspend (&Tache2_Ptr);
		
		
	}
	
}



void Tache1 (long int x)
{
	static int i = 0;
	static int j = 0;
	
	double coeff = 32767.5, Voltage, Phase, freq, x;
	double Tab[N] = {0.000000,0.125333,0.248690,0.368125,0.481754,0.587785,0.684547,0.770513,0.844328,0.904827,0.951057,0.982287,0.998027,0.998027,0.982287,0.951057,0.904827,0.844328,0.770513,0.684547,0.587785,0.481754,0.368125,0.248690,0.125333,0.000000,-0.125333,-0.248690,-0.368124,-0.481754,-0.587785,-0.684547,-0.770513,-0.844328,-0.904827,-0.951056,-0.982287,-0.998027,-0.998027,-0.982287,-0.951057,-0.904827,-0.844328,-0.770513,-0.684547,-0.587785,-0.481754,-0.368125,-0.248690,-0.125333 };
  
	lsampl_t TabCNA[N], S, f, p, a, data;

	// conversion des valeurs de sinus pour lenvoi avec le CNA
	for (i = 0 ; i < N; i++){
		TabCNA[i] = 0.5 * coeff * Tab[i] + coeff;
	}
  
	while (1)
	{
		// attente de la semaphore P
		rt_sem_wait(&S1);

		// depiler les donnees de la fifo
		rtf_get(fifo1, &n);
		rtf_get(fifo1, &a);
		rtf_get(fifo1, &S);
		rtf_get(fifo1, &f);
		rtf_get(fifo1, &p);
		rtf_get(fifo1, &data);
		
		if (a) { // demande de modification de l'amplitude "a = 1"
			
			// conversion CAN a une donnees amplitude [0 5V]
			Voltage = ( (5)/(65535) ) * data  

			// mise a jour des donnes
			for (i = 0 ; i < N; i++){
				TabCNA[i] = Voltage * 0.5 * coeff * Tab[i] + coeff;
			}
		}

		else if (f) {  // demande de modification de la frequence "f = 1"
				
			if (S){ // range de la frequence de [10 - 100] (hz) "s = 1"

				// conversion du CAN a une donnee frequence  [10 - 100] (hz)
				freq = ( (99)/(65535) ) * data + 1 ;
				
				x = (1) / (2e-6 * 50 * freq);
				rt_busy_sleep( (2*x-2) * microsec);
			}
			else{ // range de la frequence de [100 - 1e4] (hz) "s = 0"

				// conversion du CAN a une donnee frequence [100 - 1e4] (hz)
				freq = ( (9900)/(65535) ) * data + 100 ; 
				
				x = (1) / (2e-6 * 50 * freq);
				rt_busy_sleep( (2*x-2) * microsec);
			}
		}
		else if (p){ // demande de modification de la phase "p = 1"

			// conversion du CAN a une donnee phase [-pi/2 pi/2]
			Phase = ( (PI) / (65535) ) * data - (PI)/ (2) ; 

			// mise a jour des donnes
			for (i = 0 ; i < N; i++){
				TabCNA[i] =  coeff * ( Tab[i] + Phase ) + coeff;
			}

		}

		if (n) { // signal 1

			// envoie des donnes sinus dans le canal 1 (signal 1)
			comedi_data_write(carte, CNA, CHAN_1, 0, AREF_GROUND, TabCNA[j]); // write CNA i

			// mise a jour du pointeur d'echantillon
			j = (j+1) % N;
			
			// si t = T reveil de la tache modification
			if ( ( j % 50 ) == 0 )
			{
				rt_task_resume(&Tache2_Ptr);
			}

			
			// attente jusqu'a la prochaine periode
			rt_task_wait_period();
		}
		else  
		{   // signal 0

			
			// envoie des donnes sinus dans le canal 0 (signal 0)
			comedi_data_write(carte, CNA, CHAN_0, 0, AREF_GROUND, TabCNA[j]); // write CNA i
				
			// mise a jour du pointeur d'echantillon
			j = (j+1) % N;	
				
			// si t = T reveil de la tache modification
			if ( ( j % 50 ) == 0 )
			{
				rt_task_resume(&Tache2_Ptr);
			}

			// attente jusqu'a la prochaine periode
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
 	
	// creation de la fifo
	rtf_create(fifo1,2000);

	// creation de la semaphore
	rt_sem_init(&S1,1);


  	// Initialisation de la carte d'E/S
  	carte = comedi_open("/dev/comedi0");	
  	
	if(carte == NULL)
  	{
    		comedi_perror("Comedi fails to open");
    		return -1;
  	}

	// Configurer le device DIGITAL_INPUT pour recevoir les donnees/signaux
	// et DIGITAL_OUTPUT pour envoyer les donnees/signaux
	comedi_dio_config(carte,DIO,CHAN_1,COMEDI_INPUT);
	comedi_dio_config(carte,DIO,CHAN_2,COMEDI_INPUT);
	comedi_dio_config(carte,DIO,CHAN_3,COMEDI_INPUT);
	comedi_dio_config(carte,DIO,CHAN_4,COMEDI_INPUT);


	rt_set_oneshot_mode();
	//rt_set_periodic_mode();	
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, 0);
	

	// Lancement du timer
	timer_periode = start_rt_timer(nano2count(2 * microsec)); // 2 uS
	now = rt_get_time();
  	// Lancement des taches
	rt_task_make_periodic(&Tache1_Ptr, now, timer_periode*1); // 1 point toutes les 2 uS T = 0.1 ms , F = 10Khz (signal) 
	//rt_task_make_periodic(&Tache2_Ptr, now, timer_periode*50); // 1 test toute les periode
	rt_task_resume (&Tache2_Ptr);

  	return 0;
}


void cleanup_module(void)
{
  stop_rt_timer();
  // Destruction des objets de l'application
  rt_task_delete(&Tache1_Ptr);
  rt_task_delete(&Tache2_Ptr);
  rtf_destroy(fifo1);
  rt_sem_delete(&S1);
}