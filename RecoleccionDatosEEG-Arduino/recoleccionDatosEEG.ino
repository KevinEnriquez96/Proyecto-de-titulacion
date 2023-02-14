#include <SoftwareSerial.h>
SoftwareSerial BT(0,1); //Pines del Módulo Rx10/Tx11 para la comunicacion con el microcontrolador
//Declaración de la velocidad de la comunicacion serial y proceso de debug expresada en baudios por segundo
#define BAUDRATE 57600 
#define DEBUGOUTPUT 0
//Declaración de variables
byte payloadChecksum = 0; //Variable que almacena el byte para realizar la comprobacion de la carga a través del proceso de suma de errores
byte  checksum = 0; //Variable que almacena el byte del proceso del Checksum
int   payloadLength = 0;//Variable que almacena el tamaño de la carga útil
byte  payloadData[64] = {0};//Vector que almacena el tamaño total de un paquete. 
byte  poorQuality = 0;//Variable que almacena el valor de la calidad de la señan
byte  attention = 0;//Variable que almacena los valores de atención
byte  meditation = 0;//Variable que almacena los valores de meditación
// Variables del sistema
boolean bigPacket = false;//Variable lógica que permite identificar si un paquete es mayor a 170 bytes
boolean brainwave = false;//Variable lógica que permite identificar si el paquete contiene información de señales EEG

void setup() {
  //Variables que permiten iniciar la comunicacion serial
  BT.begin(BAUDRATE);           
  Serial.begin(BAUDRATE);  
}

//Funcion que permite la lectura de un byte para la suma de comprobación. 
byte ReadOneByte() {
  int ByteRead; //Variable que almacena un número entero para el proceso de suma de comprobación.
  while(!BT.available());//Inicialización del proceso para la lectura del byte que será recolectado durante el proceso de comunicación
    ByteRead = BT.read(); //Estos son datos que ya llegaron y se almacenaron en el búfer de recepción en serie (que contiene 64 bytes)

  #if DEBUGOUTPUT  
    Serial.print((char)ByteRead);   //Este proceso permite la impresión del valor almacenado por la variable ByteRead y sirve como proceso de depuración.
  #endif
  return ByteRead; //Esta función si es declarada en algun escenario, devuelve el valor almacenado en la variable ByteRead
}

// Variables que permiten almacenar los datos señales EE
unsigned int delta_wave = 0;
unsigned int theta_wave = 0;
unsigned int low_alpha_wave = 0;
unsigned int high_alpha_wave = 0;
unsigned int low_beta_wave = 0;
unsigned int high_beta_wave = 0;
unsigned int low_gamma_wave = 0;
unsigned int mid_gamma_wave = 0;
int read_3byte_int(int i) {          //
 return ((payloadData[i] << 16) + (payloadData[i+1] << 8) + payloadData[i+2]);

}
//metodo para las ondas 
void read_waves(int i) {
  //Este metodo permite realizar la construcción de los datos asociados una señal EEG
  //Hay que recalcar que cada señal estar constituida por 3 bytes, y si multiplica los 3 bytes por las  8 ondas
  //Se tendrá la cantidad del payload. En este caso constituye los 24 bytes para la revision de estas señales
  delta_wave = read_3byte_int(i);//Variable que almacena datos en relación con la señal DELTA
  i+=3;//Construcción de los 3 bytes a manera de vector en relación con datos de las señales EEG
  theta_wave = read_3byte_int(i);//Variable que almacena datos en relación con la señal THETA
  i+=3;
  low_alpha_wave = read_3byte_int(i);//Variable que almacena datos con relación a la señal ALFA baja
  i+=3;
  high_alpha_wave = read_3byte_int(i);//Variable que almacena datos en relación con la señal ALFA alta
  i+=3;
  low_beta_wave = read_3byte_int(i);//Variable que almacena datos en relación con la señal BETA baja
  i+=3;
  high_beta_wave = read_3byte_int(i);//Variable que almacena datos en relación con la señal BETA alta
  i+=3;
  low_gamma_wave = read_3byte_int(i);//Variable que almacena datos en relación con la señal GAMMA baja
  i+=3;
  mid_gamma_wave = read_3byte_int(i);//Variable que almacena datos en relación con la señal GAMMA media
}

//Metodo principal
void loop() {
// Este método realiza los procesos detallados en la FIGURA
//Inicia con la lectura de los bytes de sincronización alojados en las primeras 2 cabeceras del paquete
  if(ReadOneByte() == 170) {//Lectura del valor del primer byte de sincronización y debe corresponder a 0xAA=170(decimal)
    if(ReadOneByte() == 170 ){//Lectura del valor del segundo byte de sincronización y debe corresponder a 0xAA=170(decimal)
     
     payloadLength = ReadOneByte(); //Se iguala la variable del tamaño de la carga útil, junto con el valor de retorno de la función ReadOneByte
      if(payloadLength > 169)//Si la carga útil es mayor a 169 bytes, descarta el paquete y empieza el proceso nuevamente
      return;
      payloadChecksum = 0;        
      for(int i = 0; i < payloadLength; i++) { //Esta función definida por la función del FOR inicia la comprobación de la suma de errores
        payloadData[i] = ReadOneByte();            
        payloadChecksum += payloadData[i];
      }   
      checksum = ReadOneByte();                            
      payloadChecksum = 255 - payloadChecksum;  //Con esta funcion permite establecer un paquete dentro del rango establecido como paquete no erroneo

        if(checksum == payloadChecksum) {    
          poorQuality = 200;  //calidad de la señal si se acerca a 200 es mala. Si el paquete detecta que tiene esta cantidad, elimina el paquete
          attention = 0;//El valor de atencion seteado en 0 cuando la calidad de la senal es mala
          meditation = 0; //El valor de meditación seteado en 0 cuando la calidad de la senal es mala
        
        brainwave=false;
         for(int i = 0; i < payloadLength; i++) {    
          switch (payloadData[i]) {
          case 2: //Cabecera con el codigo 2 para la construcción del paquete con valores de la calidad de señal
            i++;            
            poorQuality = payloadData[i];
            bigPacket = true;            
            break;
          case 4:  //Cabecera con el codigo 4 para la construcción del paquete con valores de atencion
            i++;
            attention = payloadData[i];                        
            break;
          case 5: //Cabecera con el codigo 5 para la construcción del paquete con valores de meditación
            i++;
            meditation = payloadData[i];
            break;
          case 0x83: //Cabecera con el codigo 0*73 para la construcción del paquete con valores de señales EEG  
            i++;
            brainwave = true;
            byte vlen = payloadData[i];
            read_waves(i+1);
            i += vlen; // i = i + vlen    
            break;
              } // switch
        } // for loop
        }
        

  if(bigPacket) {
          //Este condicional sirve para impresión de los valores de;
          //Calidad de las señal
          //Valores de antencion
          //Valorres de meditacion
          //Valores de señales EEG
          if(poorQuality == 0){
            if(brainwave && attention > 0 && attention < 100) {
            //Serial.print("Atencion:");
            Serial.print(attention, DEC);
            Serial.print("\t");
            //Serial.print("Meditacion:");
            //Serial.print(meditation, DEC);
            //Serial.print("\t");            
            //Serial.print("Calidad sign: ");
            //Serial.print(poorQuality );
            //Serial.print("\t");
           // Serial.print(" Onda Delta ");
            Serial.print(delta_wave, DEC);
            Serial.print("\t");
            //Serial.print(" Theta: ");
            Serial.print(theta_wave, DEC);
            Serial.print("\t");
           // Serial.print(" Low Alpha: ");
            Serial.print(low_alpha_wave, DEC);
            Serial.print("\t");
           // Serial.print(" High Alpha: ");
            Serial.print(high_alpha_wave, DEC);
            Serial.print("\t");            
            //Serial.print(" low beta:");   
            Serial.print(low_beta_wave, DEC);
            Serial.print("\t"); 
            //Serial.print(" high beta: "); 
            Serial.print(high_beta_wave, DEC);
            Serial.print("\t");  
            //Serial.print("low gamma:");          
            Serial.print(low_gamma_wave, DEC);
            Serial.print("\t");
            //Serial.print(" mid gamma:");
            Serial.print(mid_gamma_wave, DEC);
            Serial.print("\n"); 
            }
          }
          else{                             // no hace nada
           }
         }
        } 
        }
      }
