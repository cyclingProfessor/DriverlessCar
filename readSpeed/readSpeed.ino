volatile int counter = 0;

void addToCounter() {
  counter += 1;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(2), addToCounter, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
  Serial.write("Rotations = ");
  Serial.println(counter / 20);
}
