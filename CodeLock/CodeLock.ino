/* Main application sketch for Code Lock */
/* Demo available on YouTube BSPEmbed   */

#include <Rotary.h>         
#include <EEPROM.h>

#define ENCODER_A       2                      
#define ENCODER_B       3                      
#define ENCODER_BTN     4                     
#define BUZZER_PIN      11  
#define RELAY_PIN       12
    
#define MAGIC_NO        44     /* For EEPROM Detection */
#define MAGIC_ADD       0       /* EEPROM ADDRESS */
#define PWD_ADDR        1
#define TRUE            1
#define FALSE           0
#define ERROR_PRESS     0
#define SHORT_PRESS     1
#define MEDIUM_PRESS    2
#define MAX_CODE        4
#define S_PRESS_TIME    250     /* in milli seconds */
#define M_PRESS_TIME    1000
#define DLY_SEC         1000    /* Display Hold for Info*/

/* Global Variables */
int8_t  Pwd[]           = {1,2,3,4};   /* Default Password */
int8_t  TempPwd[MAX_CODE];             /* Default Password */
int8_t  Code            = 0;
uint8_t CodeCnt         = 0;
bool    EncodeFlag      = FALSE;
bool    ChngPwdFlag     = FALSE;
bool    OldPwdFlag      = FALSE;
     
Rotary r = Rotary(ENCODER_A, ENCODER_B);

/* Define Macros */
#define WriteMode()     EEPROM.write(MODE_ADD, Mode) 
#define ReadMode()      EEPROM.read(MODE_ADD)

#define RelayOn()       digitalWrite(RELAY_PIN, HIGH)
#define RelayOff()      digitalWrite(RELAY_PIN, LOW)

void setup() {
  Serial.begin(9600);
  ReadEEPROM();
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  Beep(2, 75);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  PCICR |= (1 << PCIE2);                    /* Enable pin change interrupt for the encoder */
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
}

void loop() {
  /* Update for button Press */
  switch (get_button()) {
    case SHORT_PRESS:   Beep(1, 50);
                        TempPwd[CodeCnt] = Code;  
                        Code = 0;    
                        if (++CodeCnt >= MAX_CODE) {
                          CodeCnt = 0;
                          if (OldPwdFlag) {
                            OldPwdFlag = ChngPwdFlag = FALSE;
                            WritePwd(TempPwd, PWD_ADDR);
                            Beep(3,100); 
                            CopyPwd();
                          }
                          else if (VeriPwd()) 
                            if (ChngPwdFlag) {
                              OldPwdFlag = TRUE;
                              Beep(2, 150);
                            } else DoorOpen();
                          else {
                            Beep(1, 750);
                            OldPwdFlag = ChngPwdFlag = FALSE;
                          }
                        }
                        break;  
    case MEDIUM_PRESS:  Beep(1, 75); ChngPwdFlag = TRUE; break;
    default: break;
  }
  if (EncodeFlag) { 
    EncodeFlag = FALSE;
    Beep(1, 10);
  }
}
/**************************************/
/* Copy  Password From TEMP to PWD    */
/**************************************/
uint8_t CopyPwd(void) {
  uint8_t i;
  for (i = 0; i < MAX_CODE; i++)
    Pwd[i] = TempPwd[i];
}
/**************************************/
/* Verify Password                    */
/**************************************/
uint8_t VeriPwd(void) {
  uint8_t i;
  for (i = 0; i < MAX_CODE; i++)
    if (Pwd[i] != TempPwd[i])
        return 0;
   return 1;     
}
/**************************************/
/* Door Open. Activates Relay         */
/* for 2 seconds                      */
/**************************************/
void DoorOpen(void) {
  digitalWrite(RELAY_PIN, HIGH);
  Beep(1, 250);
  delay(2000);
  digitalWrite(RELAY_PIN, LOW);
} 
/**************************************/
/* Beep for Indications               */
/**************************************/
void Beep(int8_t no, uint16_t on) {
  while (no--) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(on);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
} 
/**************************************/
/* Interrupt service routine for      */
/* encoder for frequency change       */
/**************************************/
ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result == DIR_CW) {
    EncodeFlag = 1;
    Code++;
  } else if (result == DIR_CCW) {
    Code--;
    EncodeFlag = 1;
  }
}
/**************************************/
/* Read the button with debouncing    */
/**************************************/
uint8_t get_button() {
  if (!digitalRead(ENCODER_BTN)) {
    delay(20);
    if (!digitalRead(ENCODER_BTN)) {
      long strttime = millis();
      while (!digitalRead(ENCODER_BTN));
      long Duration = millis() -  strttime; 
      if (Duration > S_PRESS_TIME)
        return MEDIUM_PRESS;
      else
        return SHORT_PRESS;          
    }
  }
  return ERROR_PRESS;
}
/**************************************/
/*    Read Password From EEPROM       */
/*    If NEW, store default Password  */
/**************************************/
void ReadEEPROM(void){
 if (EEPROM.read(MAGIC_ADD) != MAGIC_NO) { /* New EEPROM & Default Values*/
      EEPROM.write(MAGIC_ADD, MAGIC_NO);
      WritePwd(Pwd, PWD_ADDR);
   } else                                   /* Read Entire Array */
       ReadPwd(Pwd, PWD_ADDR);
}
/**************************************/
/*    Write Password to EEPROM          */
/**************************************/
void WritePwd(char* Pwd, uint8_t Addr) {
  uint8_t i = Addr, j;
  for (j = 0; j < MAX_CODE; j++,i++) 
       EEPROM.write(i, Pwd[j]);  
}
/**************************************/
/* Read code From EEPROM              */
/**************************************/
void ReadPwd(char* Pwd, uint8_t Addr) {
  byte i = Addr, j;
  for (j = 0; j < MAX_CODE; j++,i++) 
      Pwd[j] = EEPROM.read(i); 
}
