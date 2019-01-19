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
// Change to using Pololo TB6612FNG Mosfet controller

// Motor 1
int dir1PinA = 7;
int dir2PinA = 8;
int speedPinA = 6; // Needs to be a PWM pin to be able to control motor speed
int enablePin = 9; // Enable Motor control

void setup() {  // Setup runs once per reset
  Serial.begin(115200);

  //Define TB6612FNG Dual H-Bridge Motor Controller Pins

  pinMode(dir1PinA, OUTPUT);
  pinMode(dir2PinA, OUTPUT);
  pinMode(speedPinA, OUTPUT);
  pinMode(enablePin, OUTPUT);
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
        digitalWrite(enablePin, HIGH);
        Serial.println("Motor 1 Forward"); // Prints out “Motor 1 Forward” on the serial monitor
        Serial.println("   "); // Creates a blank line printed on the serial monitor
        break;

      case '2': // Motor 1 Stop (Brake)
        analogWrite(speedPinA, 0);
        digitalWrite(dir1PinA, HIGH);
        digitalWrite(dir2PinA, HIGH);
        digitalWrite(enablePin, HIGH);
        Serial.println("Motor 1 Brake");
        Serial.println("   ");
        break;

      case '3': // Motor 1 Reverse
        analogWrite(speedPinA, 100);
        digitalWrite(dir1PinA, HIGH);
        digitalWrite(dir2PinA, LOW);
        digitalWrite(enablePin, HIGH);
        Serial.println("Motor 1 Reverse");
        Serial.println("   ");
        break;

      case '4': // Motor 1 Coast
        digitalWrite(enablePin, LOW);
        digitalWrite(speedPinA, LOW);
        digitalWrite(dir1PinA, LOW);
        digitalWrite(dir2PinA, LOW);
        Serial.println("Motor 1 Coast");
        Serial.println("   ");
        break;

      case '5': // Motor 1 Short Brake
        digitalWrite(speedPinA, HIGH);
        digitalWrite(dir1PinA, HIGH);
        digitalWrite(dir2PinA, HIGH);
        digitalWrite(enablePin, HIGH);
        Serial.println("Motor 1 Short Brake");
        Serial.println("   ");
        break;

      default:
        break;
    }
  }
}
