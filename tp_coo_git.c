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


static RT_TASK Tache1_Ptr; // Pointeur pour la tache 1
static RT_TASK Tache2_Ptr;
SEM S1;

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
		comedi_data_read(carte, CAN, CHAN_0, CAN_RANGE, AREF_GROUNd, &data);

		rtf_put(fifo1, &n, sizeof(n));
		rtf_put(fifo1, &a, sizeof(a));
		rtf_put(fifo1, &S, sizeof(S));
		rtf_put(fifo1, &f, sizeof(f));		
		rtf_put(fifo1, &p, sizeof(p));
		rtf_put(fifo1, &data, sizeof(data));	// valeur du CAN 
		rt_sem_signal(&S1);
		
		
	}
	
}



void Tache1 (long int x)
{
	static int i = 0;
	double coeff = 32767.5, Voltage, Phase, freq, x;
	double Tab[N] = {0.000000,0.125333,0.248690,0.368125,0.481754,0.587785,0.684547,0.770513,0.844328,0.904827,0.951057,0.982287,0.998027,0.998027,0.982287,0.951057,0.904827,0.844328,0.770513,0.684547,0.587785,0.481754,0.368125,0.248690,0.125333,0.000000,-0.125333,-0.248690,-0.368124,-0.481754,-0.587785,-0.684547,-0.770513,-0.844328,-0.904827,-0.951056,-0.982287,-0.998027,-0.998027,-0.982287,-0.951057,-0.904827,-0.844328,-0.770513,-0.684547,-0.587785,-0.481754,-0.368125,-0.248690,-0.125333 };
  
	lsampl_t TabCNA[N], S, f, p, a, data;
	
	rt_sem_wait(&S1);
  	rtf_get(fifo1, &n);
  	rtf_get(fifo1, &a);
  	rtf_get(fifo1, &S);
  	rtf_get(fifo1, &f);
  	rtf_get(fifo1, &p);
  	rtf_get(fifo1, &data);

	for (i = 0 ; i < N; i++){
		TabCNA[i] = 0.5 * coeff * Tab[i] + coeff;
	}
  
  	if (a) {

		Voltage = ( (5)/(65535) ) * data  // conversion ADC pour un registre ADC de 16 bits [0 5V]

		for (i = 0 ; i < N; i++){
			TabCNA[i] = Voltage * 0.5 * coeff * Tab[i] + coeff;
		}
	}

	else if (f) {   // rt_busy_sleep delai sans liberation du processeur
		
		if (S){ 
			
			freq = ( (99)/(65535) ) * data + 1 ; // Vmin Vmax  -> 1 100 
			x = (1) / (2e-6 * 50 * freq);
			rt_busy_sleep( (2*x-2) * microsec);
		}
		else{ // 100 - 10 khZ

			freq = ( (9900)/(65535) ) * data + 100 ; // Vmin Vmax  -> 100 10k
			x = (1) / (2e-6 * 50 * freq);
			rt_busy_sleep( (2*x-2) * microsec);

		}
	}
	else if (p){

		Phase = ( (PI) / (65535) ) * data - (PI)/ (2) ;

		for (i = 0 ; i < N; i++){
			TabCNA[i] =  coeff * ( Tab[i] + Phase ) + coeff;
		}

	}

	if (n) { // signal 1
	
		while (1)
		{  
			comedi_data_write(carte, CNA, CHAN_1, 0, AREF_GROUND, TabCNA[i]); // write CNA i

			i = (i+1) % N;	

			rt_task_wait_period();
   		}
	}

	else { // signal 0

 		while (1)
  		{  
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
 	rtf_create(1,2000);
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
	rt_task_make_periodic(&Tache2_Ptr, now, timer_periode*50); // 1 test toute les periode


  	return 0;
}


void cleanup_module(void)
{
  stop_rt_timer();
  // Destruction des objets de l'application
  rt_task_delete(&Tache1_Ptr);
  rt_task_delete(&Tache2_Ptr);
  rtf_destroy(1);
  rt_sem_delete(&S1);
}