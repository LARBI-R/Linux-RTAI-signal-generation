#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <stdio.h> /* for printf() */
#include <comedilib.h>
#include <math.h>


#define N 50
#define fifo1 1
#define fifo1 2
#define n 4
#define a 3
#define p 2
#define f 1
#define s 0
#define PI 3.14159265359
#define BIT_SET(port, bit) ( (port) & (1 << bit) ) ? 1 : 0


static RT_TASK lectureCAN;
static RT_TASK ecritureCNA;
static RT_TASK TraitementVar;

int subdev = 0; /* indique quel composant de la carte sera
utilisé */
int chan = 0; /* indique quel port ou voie sera utilisé */
int range = 0; /* indique l’échelle qui sera utilisée */
int aref = AREF_GROUND; /* indique la référence de masse à utiliser */


static void lectureCAN_code(long int X)
{
	comedi_t *ma_carte;
	lsampl_t PORTA, POTENTIO;
	ma_carte=comedi_open("/dev/comedi0");
	
	while (1)
	{
		comedi_data_read(ma_carte,subdev,chan,range,aref, &PORTA); /*lecture d’une donnée contenue dans la variable data, sur
		ma_carte, sur le composant 0, la voie 0, la première échelle 0,
		utilsant la référence AREF_GROUND, */
		
		rtf_put(fifo1, &PORTA, sizeof(PORTA));
		rt_sem_signal(&S1);

	}
	
}

static void TraitementVar_code(long int X)
{
	lsampl_t A1 = 5 , A0 = 5, f1 = 100, f0 = 100 , Phi1 = 0, Phi0 = 0, temp = 0;
	
	while (1) {
		rt_sem_wait(&S1);
        rtf_get(fifo1, &PORTA, sizeof(PORTA));
		if( BIT_SET(PORTA,n) ){ 
			temp = 2;
			rtf_put(fifo2, &temp, sizeof(temp));
			// signal 1
			if ( BIT_SET(PORTA,a) ){
				comedi_data_read(ma_carte,subdev,chan,range,aref, &POTENTIO); // Lecture du potentio
				A1 = (potentio +5.)/2.; // Décallage des valeurs du potention [-5, +5] à [0, 5]
				
				rtf_put(fifo2, &A1, sizeof(A1));
				rtf_put(fifo2, &Phi1, sizeof(Phi1));
				rtf_put(fifo2, &f1, sizeof(f1));
				rt_sem_signal(&S2);
				
			}
			else if ( BIT_SET(PORTA,p) ){
				comedi_data_read(ma_carte,subdev,chan,range,aref, &POTENTIO);
				Phi1 = potentio*3.1415/(5.*2.); // Décallage des valeurs du potentio de [-5, +5] à [-PI/2, +PI/2]
				
				rtf_put(fifo2, &A1, sizeof(A1));
				rtf_put(fifo2, &Phi1, sizeof(Phi1));
				rtf_put(fifo2, &f1, sizeof(f1));
				rt_sem_signal(&S2);
			}
			else if ( BIT_SET(PORTA,f) ){
				if (BIT_SET(PORTA,s){
					comedi_data_read(ma_carte,subdev,chan,range,aref, &POTENTIO);
					f1 = 990.*potentio + 5050; // Décallage des valeurs du potentio de [-5, 5] à [100, 10 000]
					
					rtf_put(fifo2, &A1, sizeof(A1));
					rtf_put(fifo2, &Phi1, sizeof(Phi1));
					rtf_put(fifo2, &f1, sizeof(f1));
					rt_sem_signal(&S2);
				}
				else{
					comedi_data_read(ma_carte,subdev,chan,range,aref, &POTENTIO);
					f1 = 9.9*potentio + 50.5;// Décallage des valeurs du potentio de [-5, 5] à [1, 100]
					
					rtf_put(fifo2, &A1, sizeof(A1));
					rtf_put(fifo2, &Phi1, sizeof(Phi1));
					rtf_put(fifo2, &f1, sizeof(f1));
					rt_sem_signal(&S2);
				}
			}			
		} else{ 
			// signal 0
			temp = 1;
			rtf_put(fifo2, &temp, sizeof(temp));
			rt_sem_signal(&S2);
			if ( BIT_SET(PORTA,a) ){
				comedi_data_read(ma_carte,subdev,chan,range,aref, &POTENTIO); // Lecture du potentio
				A0 = (potentio +5.)/2.; // Décallage des valeurs du potention [-5, +5] à [0, 5]
				
				rtf_put(fifo2, &A0, sizeof(A0));
				rtf_put(fifo2, &Phi0, sizeof(Phi0));
				rtf_put(fifo2, &f0, sizeof(f0));
				rt_sem_signal(&S2);
			}
			else if ( BIT_SET(PORTA,p) ){
				comedi_data_read(ma_carte,subdev,chan,range,aref, &POTENTIO);
				Phi0 = potentio*3.1415/(5.*2.); // Décallage des valeurs du potentio de [-5, +5] à [-PI/2, +PI/2]
				
				rtf_put(fifo2, &A0, sizeof(A0));
				rtf_put(fifo2, &Phi0, sizeof(Phi0));
				rtf_put(fifo2, &f0, sizeof(f0));
				rt_sem_signal(&S2);
			}
			else if ( BIT_SET(PORTA,f) ){
				
				if (BIT_SET(PORTA,s){
					comedi_data_read(ma_carte,subdev,chan,range,aref, &POTENTIO);
					f0 = 990.*potentio + 5050; // Décallage des valeurs du potentio de [-5, 5] à [100, 10 000]
					
					rtf_put(fifo2, &A0, sizeof(A0));
					rtf_put(fifo2, &Phi0, sizeof(Phi0));
					rtf_put(fifo2, &f0, sizeof(f0));
					rt_sem_signal(&S2);
				}
				else{
					comedi_data_read(ma_carte,subdev,chan,range,aref, &POTENTIO);
					f0 = 9.9*potentio + 50.5;// Décallage des valeurs du potentio de [-5, 5] à [1, 100]
					
					rtf_put(fifo2, &A0, sizeof(A0));
					rtf_put(fifo2, &Phi0, sizeof(Phi0));
					rtf_put(fifo2, &f0, sizeof(f0));
					rt_sem_signal(&S2);
				}
			}			

		
		} 
	
	}
}


static void ecritureCNA_code(long int X)
{
	lsampl_tA1 = 5 , A0 = 5, f1 = 100, f0 = 100 , Phi1 = 0, Phi0 = 0, temp, Te0, Te1;
	double S0, S1;
	
	rt_sem_wait(&S2);
	rtf_get(fifo2, &temp, sizeof(temp));

	if (temp == 1) { // demande de modification du signal 0
	
		rtf_get(fifo2, &A0, sizeof(A0));
		rtf_get(fifo2, &Phi0, sizeof(Phi0));
		rtf_get(fifo2, &f0, sizeof(f0));
		
		Te0 = 1/(10 * f0);
		
		for (int i = 0 ; i < N; ++i){
			S0 = A0 * sin(2*PI * f0 * i * Te0 + Phi0);
			comedi_data_write(ma_carte,subdev,chan,range,aref, &S0);
		}
		
		
	} else if ( temp == 2 ){ // demande de modification du  signal 1
 		
		rtf_get(fifo2, &A1, sizeof(A1));
		rtf_get(fifo2, &Phi1, sizeof(Phi1));
		rtf_get(fifo2, &f1, sizeof(f1));
		
		Te1 = 1 / (10 * f1);
		
		for (int i = 0 ; i < N; ++i){
			S1 = A1 * sin(2*PI * f1 * i * Te1 + Phi1);
			comedi_data_write(ma_carte,subdev,chan,range,aref, &S1);
		}
		
		
		
	}
	else { // pas de demande de modification
	
		Te0 = 1/(10*f0);
		Te1 = 1/(10*f1);
		
		S0 = A0 * sin(2*PI * f0 * i * Te0 + Phi0);
		S1 = A1 * sin(2*PI * f1 * i * Te1 + Phi1);
		
		comedi_data_write(ma_carte,subdev,chan,range,aref, &S0);
		comedi_data_write(ma_carte,subdev,chan1,range,aref, &S1);
		
	}
	
	
}