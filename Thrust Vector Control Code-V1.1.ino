#include <Wire.h>
#include <Servo.h>
#include <I2Cdev.h>
#include <MPU6050.h>

Servo XServo;
Servo YServo;

MPU6050 MPU1;

int16_t Acc_rawX, Acc_rawY, Acc_rawZ; //Accel constants
int16_t Gyr_rawX, Gyr_rawY, Gyr_rawZ; //Gyro constants

float Acceleration_angleX;
float Acceleration_angleY;
float Gyro_angleX;
float Gyro_angleY;
float Total_angleX;
float Total_angleY;

float elapsedTime, time, timePrev;
float rad_to_deg = 180 / 3.141592654;

float PID_x, PID_y, error_x, error_y, previous_error_x, previous_error_y;
float pid_p_x = 0;
float pid_p_y = 0;
float pid_i_x = 0;
float pid_i_y = 0;
float pid_d_x = 0;
float pid_d_y = 0;

float kp = 1.0; // Proportional constant
float ki = 0.0001; // Integral constant
float kd = 0.001; // Derivative constant

float desired_angle = 0; // Desired TVC Angle
float ServoStartAngle = 90;

void setup() {
  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(19200);
  myServoX.attach(9);
  myServoY.attach(11);
  myServoX.write(ServoStartAngle);
  myServoY.write(ServoStartAngle);
  time = millis(); // Start counting time in milliseconds
}

void loop() {
  timePrev = time; // Initial time stored before the actual time is read
  time = millis(); // Actual time
  elapsedTime = (time - timePrev) / 1000;

  // Request raw accelerometer data
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6, true);

  Acc_rawX = Wire.read() << 8 | Wire.read();
  Acc_rawY = Wire.read() << 8 | Wire.read();
  Acc_rawZ = Wire.read() << 8 | Wire.read();

  // Calculate Euler angles from accelerometer data (X, Y)
  Acceleration_angleX = atan((Acc_rawY / 16384.0) / sqrt(pow((Acc_rawX / 16384.0), 2) + pow((Acc_rawZ / 16384.0), 2))) * rad_to_deg;
  Acceleration_angleY = atan(-1 * (Acc_rawX / 16384.0) / sqrt(pow((Acc_rawY / 16384.0), 2) + pow((Acc_rawZ / 16384.0), 2))) * rad_to_deg;

  // Request raw gyroscope data
  Wire.beginTransmission(0x68);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 4, true);

  Gyr_rawX = Wire.read() << 8 | Wire.read();
  Gyr_rawY = Wire.read() << 8 | Wire.read();

  // Gyroscope angles (in degrees)
  Gyro_angleX = Gyr_rawX / 131.0;
  Gyro_angleY = Gyr_rawY / 131.0;

  // Apply complementary filter to calculate total angles (X and Y)
  Total_angleX = 0.98 * (Total_angleX + Gyro_angleX * elapsedTime) + 0.02 * Acceleration_angleX;
  Total_angleY = 0.98 * (Total_angleY + Gyro_angleY * elapsedTime) + 0.02 * Acceleration_angleY;

  // Calculate the error between desired angle and actual measured angle
  error_x = Total_angleX - desired_angle;
  error_y = Total_angleY - desired_angle;

  // Proportional value
  pid_p_x = kp * error_x;
  pid_p_y = kp * error_y;

  // Integral value (only add if error is small enough to prevent wind-up)
  if (error_x > -100 && error_x < 100) {
    pid_i_x = pid_i_x + (ki * error_x);
  }

  if (error_y > -100 && error_y < 100) {
    pid_i_y = pid_i_y + (ki * error_y);
  }

  // Derivative value
  pid_d_x = kd * ((error_x - previous_error_x) / elapsedTime);
  pid_d_y = kd * ((error_y - previous_error_y) / elapsedTime);

  // Calculate the PID output
  PID_x = pid_p_x + pid_i_x + pid_d_x;
  PID_y = pid_p_y + pid_i_y + pid_d_y;

  // Constrain the PID output to ensure servo stays within valid range (0-180 degrees)
  int servoXPos = constrain(ServoStartAngle - PID_x, 0, 180);
  int servoYPos = constrain(ServoStartAngle - PID_y, 0, 180);

  // Output to servos
  myServoX.write(servoXPos);
  myServoY.write(servoYPos);

  // Print PID values for debugging
  Serial.println("PID Value: ");
  Serial.print("PID_x: ");
  Serial.println(PID_x);
  Serial.print("PID_y: ");
  Serial.println(PID_y);

  // Update previous errors for the next loop
  previous_error_x = error_x;
  previous_error_y = error_y;
}
