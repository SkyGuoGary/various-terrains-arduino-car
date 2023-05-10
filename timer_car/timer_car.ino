// 使用定时器进行pidMotorControl
#include <Servo.h>
#include <Wire.h>         //调用IIC库函数
#include "MH_TCS34725.h"  //调用颜色识别传感器库函数
#ifdef __AVR__
//#include <avr/power.h>
#endif
#define grayScale1 A2
#define grayScale2 A3
#define grayScale3 A0  // middle
#define motor1L 5
#define motor1R 6
#define motor2L 9
#define motor2R 10
#define SERVO 11

// 常量
const uint8_t SPEED = 250;
const uint8_t theta = 0;  // 左右轮速度差
uint8_t deltaPWM = 230;
uint8_t weakPWM = 25;
const uint8_t time_delay = 10;

//初始化目标速度
int expectedPWML = SPEED - theta;
int expectedPWMR = SPEED;
int beginSpeed = 50;
int stopSpeed = 130;

//采集到黑线时，传感器为低电平
uint8_t gS1 = 1;
uint8_t gS2 = 1;
uint8_t gS = 1;
uint8_t le = 0;
uint8_t ri = 0;
uint8_t mi = 0;
uint8_t flag = 0;
const uint8_t thresholdL = 195;
const uint8_t thresholdR = 200;
const uint8_t thresholdM = 205;
int gSL[2] = { 195, 195 };
int gSR[2] = { 200, 200 };
int gSM[2] = { 200, 200 };

// 颜色识别相关
// 卡片颜色
uint16_t red, green, blue, sum;
float r, g, b;
uint8_t color = 0, card_color = 0;
// 颜色阈值
uint16_t thresh_red = 3000;
uint16_t thresh_green = 1600;
uint16_t thresh_blue = 2500;
// 颜色传感器
MH_TCS34725 tcs = MH_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);  // 50ms

// 初始化舵机
Servo my_servo;
// 舵机位置
uint8_t DOWN = 0, UP = 180;

// 时间定时器
uint16_t upperBound = 1650;
uint16_t time_counter = 0;
uint16_t sensor_delay = 750;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(grayScale1, INPUT);
  pinMode(grayScale2, INPUT);
  pinMode(grayScale3, INPUT);
  pinMode(motor1L, OUTPUT);
  pinMode(motor1R, OUTPUT);
  pinMode(motor2L, OUTPUT);
  pinMode(motor2R, OUTPUT);
  pinMode(SERVO, OUTPUT);
  expectedPWML -= beginSpeed;
  expectedPWMR -= beginSpeed;
}

void loop() {

  // if (time_counter == upperBound - 100) {
  //   if (!tcs.begin()) {
  //     while (1)
  //       ;
  //   }
  // }
  // if (time_counter >= upperBound) hitBall();
  // if (expectedPWMR <= SPEED) {
  //   expectedPWML += 1;
  //   expectedPWMR += 1;
  // }
  // pidMotorControl();
  // ++time_counter;
  // delay(time_delay);
  colorDetect();
}

void hitBall() {
  color = colorDetect();
  if (color != 0 && card_color == 0) {
    expectedPWML -= stopSpeed;
    expectedPWMR -= stopSpeed;
    deltaPWM -= stopSpeed;
    card_color = color;  // 记录色卡颜色
    color = 0;
    motorRun(0, 0);
    delay(150);
    for (uint8_t i = 0; i < 8; ++i) 
      motorRun(0, 0);
  } 
  else if (card_color != 0 && color == card_color) {
    for (uint8_t i = 0; i < 30; ++i) 
      motorRun(0, 0);
    my_servo.attach(SERVO);
    my_servo.write(UP);
    delay(sensor_delay);
    my_servo.write(DOWN);
    delay(sensor_delay);
    my_servo.detach();
    expectedPWML += stopSpeed;
    expectedPWMR += stopSpeed;
    deltaPWM += stopSpeed;
  }
}

void motorRun(int left, int right) {
  left = left > 255 ? 255 : left;
  left = left <= 0 ? 0 : left;
  right = right > 255 ? 255 : right;
  right = right <= 0 ? 0 : right;
  analogWrite(motor1L, left);
  analogWrite(motor2R, right);
}
uint8_t colorDetect() {
  tcs.getRGBC(&red, &green, &blue, &sum);
  tcs.lock();
  // 提取颜色值
  r = red;
  r /= sum;
  g = green;
  g /= sum;
  b = blue;
  b /= sum;
  // 颜色转换
  red = transform(r);
  green = transform(g);
  blue = transform(b);
  // Serial.println(green);
  Serial.print(red); Serial.print("\t"); 
  Serial.print(green); Serial.print("\t"); 
  Serial.print(blue);
  if (red > green && red > blue && red > thresh_red) {
    return 1;
  }
  if (green > red && green > blue && green > thresh_green) {
    return 2;
  }
  if (blue > red && blue > green && blue > thresh_blue) {
    return 3;
  } else
    return 0;
}

uint16_t transform(float a) {
  uint16_t ans = exp(10 * a) * 10;
  if (ans >= 10000)
    return 0;
  else
    return ans;
}

void maintance() {
  switch (flag) {
    case 0:
      motorRun(expectedPWML, expectedPWMR);
      break;
    case 3:
      motorRun(expectedPWML, expectedPWMR);
      break;
    case 2:
      motorRun(expectedPWML - 5 * weakPWM, expectedPWMR - weakPWM);
      break;
    case 1:
      motorRun(expectedPWML - weakPWM + theta, expectedPWMR - 5 * weakPWM);
      break;
    case 5:
      motorRun(expectedPWML - deltaPWM, expectedPWMR);
      break;
    case 4:
      motorRun(expectedPWML + theta, expectedPWMR - deltaPWM);
      break;
    default:
      break;
  }
}

void pidMotorControl() {
  gSL[le] = analogRead(grayScale1);
  gSR[ri] = analogRead(grayScale2);
  gSM[mi] = analogRead(grayScale3);
  //    Serial.print(0.6 * gSL[le] + 0.4 * gSL[!le]); Serial.print(",");
  //    Serial.print(0.6 * gSR[ri] + 0.4 * gSR[!ri]); Serial.print(",");
  //    Serial.println(0.6 * gSM[mi] + 0.4 * gSM[!mi]);
  gS1 = (0.6 * gSL[le] + 0.4 * gSL[!le]) > thresholdL ? 1 : 0;
  gS2 = (0.6 * gSR[ri] + 0.4 * gSR[!ri]) > thresholdR ? 1 : 0;
  gS = (0.6 * gSM[mi] + 0.4 * gSM[!mi]) > thresholdM ? 1 : 0;
  le = !le;
  ri = !ri;
  if (gS == 0) {
    if (gS1 == 1 && gS2 == 1) {
      flag = 0;
      motorRun(expectedPWML, expectedPWMR);
    } else if (gS1 == 1 && gS2 == 0) {
      flag = 1;
      motorRun(expectedPWML - weakPWM + theta, expectedPWMR - 5 * weakPWM);
    } else if (gS1 == 0 && gS2 == 1) {
      flag = 2;
      motorRun(expectedPWML - 5 * weakPWM, expectedPWMR - weakPWM);
    } else {
      flag = 3;
      motorRun(expectedPWML, expectedPWMR);
    }  // 全黑
  } else {
    if (gS1 == 1 && gS2 == 1) {
      maintance();
    } else if (gS1 == 1 && gS2 == 0) {
      flag = 4;
      motorRun(expectedPWML + theta, expectedPWMR - deltaPWM);
    } else if (gS1 == 0 && gS2 == 1) {
      flag = 5;
      motorRun(expectedPWML - deltaPWM, expectedPWMR);
    } else {
      maintance();
    }
  }
}
