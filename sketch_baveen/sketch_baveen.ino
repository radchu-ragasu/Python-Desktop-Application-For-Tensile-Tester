#define dirPin 2
#define stepPin 3
#define limitSwitchPin 5

#define greenLED 6
#define redLED 7
#define yellowLED 8
#define buzzerPin 9

const int stepsPerRevolution = 800;
const float leadScrewLeadMM = 2.0;  // T10, 2 mm pitch, 1-start

long stepCounter = 0;
long limitSwitchReturnSteps = 0;
long targetSteps = 0;
bool isTargetMode = false;

float lastRevolutions = 0;
String lastMovement = "None";

enum MotorState { IDLE, MOVING_BACKWARD, MOVING_FORWARD, STOPPED, RETURN_FROM_LIMIT };
MotorState motorState = IDLE;

void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(limitSwitchPin, INPUT_PULLUP);

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(dirPin, LOW);
  updateLEDs();

  Serial.begin(9600);
  Serial.println("Stepper Serial Control Ready.");
  Serial.println("Commands:");
  Serial.println("  start rev 2     --> Move 2 revs backward");
  Serial.println("  back mm 6       --> Move 6 mm forward");
  Serial.println("  stop            --> Stop motor");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("start") || command.startsWith("back")) {
      bool isBackward = command.startsWith("start");
      digitalWrite(dirPin, isBackward ? LOW : HIGH);
      stepCounter = 0;
      isTargetMode = false;
      targetSteps = 0;

      float inputValue = 0;

      if (command.indexOf("rev") > 0) {
        inputValue = command.substring(command.indexOf("rev") + 3).toFloat();
        targetSteps = inputValue * stepsPerRevolution;
        isTargetMode = true;
        Serial.print("🔄 Moving ");
        Serial.print(isBackward ? "backward" : "forward");
        Serial.print(" for ");
        Serial.print(inputValue);
        Serial.println(" revolutions.");
      } else if (command.indexOf("mm") > 0) {
        inputValue = command.substring(command.indexOf("mm") + 2).toFloat();
        float revs = inputValue / leadScrewLeadMM;
        targetSteps = revs * stepsPerRevolution;
        isTargetMode = true;
        Serial.print("📏 Moving ");
        Serial.print(isBackward ? "backward" : "forward");
        Serial.print(" for ");
        Serial.print(inputValue);
        Serial.println(" mm.");
      } else {
        Serial.println("⚠️ Invalid format. Use: start/back rev/mm <value>");
      }

      motorState = isBackward ? MOVING_BACKWARD : MOVING_FORWARD;
      updateLEDs();

    } else if (command == "stop") {
      lastRevolutions = (float)abs(stepCounter) / stepsPerRevolution;
      lastMovement = (motorState == MOVING_BACKWARD) ? "Backward" : "Forward";

      Serial.print("🛑 Stopping. Total steps moved: ");
      Serial.println(stepCounter);
      Serial.print("Total revolutions (" + lastMovement + "): ");
      Serial.println(lastRevolutions);
      Serial.print("Total mm moved: ");
      Serial.println(lastRevolutions * leadScrewLeadMM);

      motorState = STOPPED;
      isTargetMode = false;
      updateLEDs();

    } else {
      Serial.println("Unknown command. Use: start/back rev/mm <value> | stop");
    }
  }

  if (digitalRead(limitSwitchPin) == LOW && motorState != RETURN_FROM_LIMIT) {
    Serial.println("⚠️ Limit switch triggered! Reversing 1 rev and stopping...");

    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);

    motorState = RETURN_FROM_LIMIT;
    digitalWrite(dirPin, HIGH);  // Move forward to reverse from switch
    limitSwitchReturnSteps = stepsPerRevolution;
    updateLEDs();
  }

  switch (motorState) {
    case MOVING_BACKWARD: moveStepBackward(); break;
    case MOVING_FORWARD: moveStepForward(); break;
    case RETURN_FROM_LIMIT: returnFromLimitSwitch(); break;
    default: break;
  }
}

void moveStepBackward() {
  if (isTargetMode && abs(stepCounter) >= targetSteps) {
    Serial.println("✅ Target backward movement complete.");
    stopAndRecord("Backward");
    return;
  }

  digitalWrite(stepPin, HIGH);
  delayMicroseconds(1250);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(1250);
  stepCounter++;

  if (stepCounter % stepsPerRevolution == 0) {
    float revs = (float)stepCounter / stepsPerRevolution;
    float mm = revs * leadScrewLeadMM;
    Serial.print("🔁 Backward: ");
    Serial.print(revs);
    Serial.print(" revs, ");
    Serial.print(mm);
    Serial.println(" mm");
  }
}

void moveStepForward() {
  if (isTargetMode && abs(stepCounter) >= targetSteps) {
    Serial.println("✅ Target forward movement complete.");
    stopAndRecord("Forward");
    return;
  }

  digitalWrite(stepPin, HIGH);
  delayMicroseconds(1250);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(1250);
  stepCounter--;

  if (abs(stepCounter) % stepsPerRevolution == 0) {
    float revs = (float)abs(stepCounter) / stepsPerRevolution;
    float mm = revs * leadScrewLeadMM;
    Serial.print("🔁 Forward: ");
    Serial.print(revs);
    Serial.print(" revs, ");
    Serial.print(mm);
    Serial.println(" mm");
  }
}

void returnFromLimitSwitch() {
  if (limitSwitchReturnSteps > 0) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1250);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1250);
    stepCounter--;
    limitSwitchReturnSteps--;

    if (limitSwitchReturnSteps % stepsPerRevolution == 0) {
      float revs = (float)limitSwitchReturnSteps / stepsPerRevolution;
      Serial.print("↩️ Returning from limit. Remaining revs: ");
      Serial.println(revs);
    }
  } else {
    Serial.println("✅ Reversed 1 rev from limit switch.");
    Serial.println("🛑 Motor stopped due to limit switch.");
    motorState = STOPPED;
    isTargetMode = false;
    updateLEDs();
  }
}

void stopAndRecord(String direction) {
  lastRevolutions = (float)abs(stepCounter) / stepsPerRevolution;
  lastMovement = direction;
  Serial.print("🔚 Final position: ");
  Serial.print(lastRevolutions);
  Serial.print(" revs, ");
  Serial.print(lastRevolutions * leadScrewLeadMM);
  Serial.println(" mm");
  motorState = STOPPED;
  updateLEDs();
}

void updateLEDs() {
  digitalWrite(greenLED, motorState == MOVING_BACKWARD);
  digitalWrite(redLED, motorState == IDLE || motorState == STOPPED);
  digitalWrite(yellowLED, motorState == MOVING_FORWARD || motorState == RETURN_FROM_LIMIT);
}
