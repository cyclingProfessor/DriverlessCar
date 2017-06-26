//Code by Reichenstein7 (thejamerson.com)

//Keyboard Controls:
//
// 1 -Motor 1 Left
// 2 -Motor 1 Stop
// 3 -Motor 1 Right
//
// 4 -Motor 2 Left
// 5 -Motor 2 Stop
// 6 -Motor 2 Right

// Declare L298N Dual H-Bridge Motor Controller directly since there is not a library to load.

// Motor 1
int dir1PinA = 7;
int dir2PinA = 8;
int speedPinA = 6; // Needs to be a PWM pin to be able to control motor speed

void setup() {  // Setup runs once per reset
  Serial.begin(115200);

  //Define L298N Dual H-Bridge Motor Controller Pins

  pinMode(dir1PinA, OUTPUT);
  pinMode(dir2PinA, OUTPUT);
  pinMode(speedPinA, OUTPUT);
}

void loop() {

  // Initialize the Serial interface:

  if (Serial.available() > 0) {
    int inByte = Serial.read();
    int speed; // Local variable

    switch (inByte) {

      //______________Motor 1______________

      case '1': // Motor 1 Forward
        analogWrite(speedPinA, 100);//Sets speed variable via PWM
        digitalWrite(dir1PinA, LOW);
        digitalWrite(dir2PinA, HIGH);
        Serial.println("Motor 1 Forward"); // Prints out “Motor 1 Forward” on the serial monitor
        Serial.println("   "); // Creates a blank line printed on the serial monitor
        break;

      case '2': // Motor 1 Stop (Freespin)
        analogWrite(speedPinA, 0);
        digitalWrite(dir1PinA, LOW);
        digitalWrite(dir2PinA, HIGH);
        Serial.println("Motor 1 Stop");
        Serial.println("   ");
        break;

      case '3': // Motor 1 Reverse
        analogWrite(speedPinA, 100);
        digitalWrite(dir1PinA, HIGH);
        digitalWrite(dir2PinA, LOW);
        Serial.println("Motor 1 Reverse");
        Serial.println("   ");
        break;

      default:
        break;
    }
  }
}
