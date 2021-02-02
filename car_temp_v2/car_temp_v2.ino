// Task setup
// 1 : COMPLETE : take care of buttons + LEDS
// 2 : encoder + motor
// 3 : COMPLETE : temperature changing 2
// 4 : screen

#include <Encoder.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include <DHT.h>

// BEGIN OF BUTTONS + LEDS INIT

// init of hardwired components

const int button_1 = 5;
const int button_2 = 13;
const int button_3 = 4;

const int led_b1 = 6;
const int led_b2 = 7;
const int led_b3 = 8;

// vars for change

int button_1_state = 0;
int button_2_state = 0;
int button_3_state = 0;

ToggleButton
  button_1_toggle(button_1),
  button_2_toggle(button_2),
  button_3_toggle(button_3);

// END OF BUTTONS + LEDS INIT
// BEGIN OF ENCODER + MOTOR

#define CLK 2
#define DT 3
#define SW 999

long oldRotaryPos = 0;
int oldButtonPos = -1;

Encoder encoder(CLK, DT);

// END OF ENCODER + MOTOR
// BEGIN TEMPERATURE SENSORS

#define DHTTYPE DHT11
#define DHTPIN 9
DHT dht(DHTPIN, DHTTYPE);

// END TEMPERATURE SENSORS
void setup() {
  //init serial communication at 9600 bits per second:
  Serial.begin(9600);
  Serial.println("Startup ... ");
  //init testing LEDs for diagnosis
  digitalWrite(led_b1, HIGH);
  digitalWrite(led_b2, HIGH);
  digitalWrite(led_b3, HIGH);
  delay(500);
  digitalWrite(led_b1, LOW);
  digitalWrite(led_b2, LOW);
  digitalWrite(led_b3, LOW);
  //init the LED pins as outputs:
  pinMode(led_b1, OUTPUT);
  pinMode(led_b2, OUTPUT);
  pinMode(led_b3, OUTPUT);
  //init the button pins as inputs:
  pinMode(button_1, INPUT);
  pinMode(button_2, INPUT);
  pinMode(button_3, INPUT);
  //JC Button setup to begin reading button for toggle
  button_1_toggle.begin();
  button_2_toggle.begin();
  button_3_toggle.begin();
  //Begin temperature sensor
  dht.begin();
}

void loop() {
  button_read();
  encoder_read();
  temperature_read();
}

void temperature_read(){
  if(! isnan(dht.readTemperature())){
    //Serial.println(dht.computeHeatIndex(dht.readTemperature(true), dht.readHumidity()));
  }
}

void button_read() {
  /*Information:
    Button 1:
      -Far left
      -Torso vent only
      -Connects to button 2 (leg)
    Button 2:
      -Middle vent
      -Leg vent only
      -Can connect to button 1 (Torso) + 3 (Window)
    Button 3:
      -Far right vent
      -Window vent only
      -Can connect to button 3 (Legs)
      
    Requirements:
      - Button combos (1 and 2) + (3 and 2) can be on simultaneously to have both of their vents on
      - Non-combo buttons can't be on at the same time (1 and 3)
      - If a button is 'active' then their respective LED should be illuminated (HIGH)
      - Write to EEPROM to save on the case of power failure (car off)
      - Flash possible combo button to show user possible combination

      - Button combinations should save to address 0 of EEPROM:
        - 0 (default if failure): Torso only (Button 1) ( 1 0 0 )
        - 1 : Middle vent only (button 2) ( 0 1 0 )
        - 2 : Window vent only (button 3) ( 0 0 1 )
        - 3 : Button 1 + 2 ( 1 1 0 )
        - 4 : Button 3 + 2 ( 0 1 1 )
  */

  button_1_toggle.read();
  button_2_toggle.read();
  button_3_toggle.read();

  int previous_button = EEPROM.read(0); // reads on startup
  
  if(button_1_toggle.changed()){
    EEPROM.update(0,0);
    if(previous_button == 1){
      EEPROM.update(0,3);
    }else if(previous_button == 3){
      EEPROM.update(0,1);
    }
  }
  if(button_2_toggle.changed()){
    if(previous_button == 3){
      EEPROM.update(0,0);
    }else if(previous_button == 4){
      EEPROM.update(0,2);
    }else if(previous_button == 0){
      EEPROM.update(0, 3);
    }else if(previous_button == 2){
      EEPROM.update(0, 4);
    }else{
      EEPROM.update(0, 1);
    }
  }
  if(button_3_toggle.changed()){
    EEPROM.update(0,2);
    if(previous_button == 1){
      EEPROM.update(0, 4);
    }else if(previous_button == 4){
      EEPROM.update(0, 1);
    }
  }

  
  previous_button = EEPROM.read(0);
  
  if(oldButtonPos != previous_button){
    if(previous_button == 0){
      Serial.println("User fan control: 0 [Torso]");
      digitalWrite(led_b1, HIGH);
      digitalWrite(led_b2, LOW);
      digitalWrite(led_b3, LOW);
    }else if (previous_button == 1){
      Serial.println("User fan control: 1 [Leg]");
      digitalWrite(led_b1, LOW);
      digitalWrite(led_b2, HIGH);
      digitalWrite(led_b3, LOW);
    }else if (previous_button == 2){
      Serial.println("User fan control: 2 [Windshield]");
      digitalWrite(led_b1, LOW);
      digitalWrite(led_b2, LOW);
      digitalWrite(led_b3, HIGH);
    } else if (previous_button == 3){
      Serial.println("User fan control: 3 [Leg and torso]");
      digitalWrite(led_b1, HIGH);
      digitalWrite(led_b2, HIGH);
      digitalWrite(led_b3, LOW);
    }else if (previous_button == 4){
      Serial.println("User fan control: 4 [Windshield and leg]");
      digitalWrite(led_b1, LOW);
      digitalWrite(led_b2, HIGH);
      digitalWrite(led_b3, HIGH);
    }
    oldButtonPos = previous_button;
  }
}

void encoder_read() {
  /*
   * Each 'click' is 4 positions
   */
  int user_temperature = EEPROM.read(1); // reads stored temperature
  long newRotaryPos = encoder.read() / 4; //divide by 4 to fix encoder skipping 4 per click
  if(newRotaryPos != oldRotaryPos){
    Serial.print("User temperature now set to: ");Serial.println(user_temperature);
    if((newRotaryPos > oldRotaryPos)&&(user_temperature>10)){
      user_temperature = user_temperature - 1;
    }else if (user_temperature<100){
      user_temperature = user_temperature + 1;
    }
    if(user_temperature>100 || user_temperature<10){
      user_temperature = 70;
    }
    EEPROM.put(1, user_temperature);
    oldRotaryPos = newRotaryPos;
  }
}
