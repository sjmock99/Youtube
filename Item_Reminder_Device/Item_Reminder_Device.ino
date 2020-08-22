//Stephen Mock
//Version 1.0 
// 7/26/19
// Youtube Video: https://www.youtube.com/watch?v=bIIDHT86DI4&feature=youtu.be

//Setting Constants for Electronics

//Ultrasonic 1 
  #define trigPin1 13
  #define echoPin1 12

//Ultrasonic 2
  #define trigPin2 9
  #define echoPin2 8

//RGB LED
  #define bluePin 6
  #define greenPin 5
  #define redPin 3

//Buzzer
  #define buzzer 2


//Setting Constants for CODE
  int state_fsm = 0;  // Starting State is 0 
// Timers
  unsigned long timer1_start = 0; //Timer for state (1)
  unsigned long timer1_check = 0; //Timer used to check if 5 minutes has passed, to go from state (2) to state (0) 
  unsigned long timer2_start = 0; // Timer inbetween state (2)
  unsigned long timer2_check = 0; // Timer to wait 5s to allow for transitions from state (3) to state (4) (Quality of Life) 
  unsigned long timer3_start = 0; // Timer for state (4)
  unsigned long timer3_check = 0; // Timer used to check if 10s passed to go from state (5) to state (6)

// Distances (EDIT for your situation!)
  int distance1Threshold = 5; // threshold before ultrasonic 1 triggers (in cm)
  int distance2Threshold = 75; //  threshold before ultrasonic 2 triggers (in cm)

// Wait Times (EDIT for your situation!) 
  unsigned long state1WaitTime = 1000*5*60; // 5 minute timer if you take the object, but dont leave the room
  unsigned long state5WaitTime = 10000; // 10s timer when you return to the room, but dont put back the object 


// Functions
// Purpose of these functions is to make the code much more readable, and to encapsulate many of the functions in the main loop into single lines. Modularity!


  // This function records the distance measured in cm as calculated from the ultrasonic sensor 
  float recordDist(int trigPin,int echoPin) { 
    float duration, distance;
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2);
   
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    duration = pulseIn(echoPin, HIGH);
    distance = (duration / 2) * 0.0344;
    return distance; // This final number is what is important for the rest of the code...
  }
  
  
  // Function to set the color of RGB LED when in different states
  void setColor(int red, int green, int blue) 
  {
    analogWrite(redPin, red);
    analogWrite(greenPin, green);
    analogWrite(bluePin, blue);
  }
  
  //Function to beep when there is an alarm/switching between states
  void beep(int del) // del = length of delay, used to differentiate sounds for active buzzer
  {
    for(int i=0;i<25;i++)
          {
          digitalWrite(buzzer,HIGH);
          delay(del);//wait for del time
          digitalWrite(buzzer,LOW);
          delay(del);//wait for del time
          }
  }



// Just setting up the various pins needed... nothing special
  void setup() {
    Serial.begin (9600); // Can you use the serial if you want!
    pinMode(trigPin1, OUTPUT);
    pinMode(echoPin1, INPUT);
    pinMode(trigPin2, OUTPUT);
    pinMode(echoPin2, INPUT);
    pinMode(redPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(buzzer,OUTPUT);
  }


void loop() {
  float distance1,distance2;

  // Sensor 1

   distance1 =  recordDist(trigPin1, echoPin1);
   Serial.print("Distance 1 = "); // Use the distance measured by serial to determine the thresholds for when you want the ultrasonics to trigger
   Serial.print(distance1);
   Serial.print(" cm :");


  // Sensor 2 

   distance2 = recordDist(trigPin2, echoPin2);
   Serial.print("Distance 2 = ");
   Serial.print(distance2);
   Serial.println(" cm");

    
   SM_alarm(distance1,distance2); // Calling the state machine!
   
   delay(50); // Need a delay otherwise recording to fast and weird data might occur
}


// STATE MACHINE!
// State machine has 7 different states (indexing at 0), and is the behavior is described in the video.
// This state machine uses a switch statement to differentiate each state, and when transitions occur, the state_fsm changes accordingly.

  void SM_alarm(float distance1, float distance2) {
    Serial.print("Current FSM State: "); // Great for debugging, to see what state you are in.
    Serial.println(state_fsm);
    
    switch (state_fsm) {  
      
        case 0: //resting state, waiting for keys to be removed, but will alarm when user leaves the room without the object
        setColor(0,255,0); // Green LED
    
        if (distance1 > distance1Threshold) { //ultrasonic detects distance greater than 3 cm, meaning the object has moved away, transition to state 1 
          state_fsm = 1;
        }
  
        if (distance2 < distance2Threshold) { // buzzer if the user passes by the second ultrasonic, to remind them to take their object before leaving
          beep(2);
        }
        break;
      
      case 1: // timer #1 start
      
        timer1_start = millis(); //Goes directly to next state, 1 == 1
        state_fsm = 2;
        break;
      
      case 2: // Detection State 1: check if 5minutes has passed with no transitions, OR go back to state 0 if Ultrasonic1 detects the object have been placed back

  
        setColor(0,0,255); // Outputs Yellow
        
        if(distance1 < distance1Threshold) { // detects that the object is back, goes to intial resting state
          state_fsm = 0;
        }
    
        timer1_check = millis(); // running clock to see if 5 minutes have passed
        
        if(timer1_check - timer1_start > state1WaitTime){ // If 5 minutes have passed
          state_fsm = 6; // go to the alarm phase
        }
    
        if(distance2 < distance2Threshold) {  //If the second Ultrasonic gets passed by, go to waiting phase state (3)
          beep(3); // acknowledging the user has left
          state_fsm = 3;
          timer2_start = millis(); // must wait 5 seconds before able to go to detection 2 (state (5)) phase from waiting phase (state (3))
          // This is needed because it takes a few seconds (more than a 50ms delay...) to pass by something, especially like your front door. 
        }
        break;
        
      case 3: // Waiting Period: when owner is out of the house with the object, device waits for the owner to return 
      
        setColor(153,0,153); // Output Purple
        
        if(distance1 < distance1Threshold) { // detects that the object is back, goes to intial resting state
            state_fsm = 0;
        }
  
        timer2_check = millis(); // running clock to see if 5s passed
        
        if(distance2 < distance2Threshold && (timer2_check - timer2_start > 5000)) { // when the owner returns to the house, and when 5s has passed
          beep(4);
          state_fsm = 4; 
        }
    
        break;
        
      case 4: // timer #3 start
        timer3_start = millis(); //Goes directly to next state (5) 1 == 1
        state_fsm = 5;
        break;
        
      case 5: // Detection State 2: Waiting for keys to return within 10s, otherwise alarming!
        setColor(255,128,0); // Outputs orange color
  
        if(distance1 < distance1Threshold) { // detects that the object is back, goes to intial resting state
            state_fsm = 0;
        }
  
        timer3_check = millis();  // Timer checking if 10s has passed, and if so, transitioning to alarm phase (6)    
        if(timer3_check - timer3_start > state5WaitTime){ // If 10 seconds have passed and user doesnt put back item
          state_fsm = 6; // go to the alarm phase!
        }
        
      break;
       
      case 6: //Alarm phase: Simply beeping rapidly until the object is placed back.
        setColor(255,0,0); // output red
        beep(1);   
        if(distance1 < distance1Threshold) { // detects that the object is back, goes to intial resting state
            state_fsm = 0;
        }  
           
      break;
    }
  }
