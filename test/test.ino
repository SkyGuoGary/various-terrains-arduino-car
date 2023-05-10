// #include <Servo.h>
#include <Wire.h>         //调用IIC库函数
#include "MH_TCS34725.h"  //调用颜色识别传感器库函数
#include "Timer2ServoPwm.h"
#ifdef __AVR__
// #include <avr/power.h>
#endif
#define gray_right A2
#define gray_left A3
#define gray_mid A0

#define motorR1 5
#define motorR2 6
#define motorL1 9
#define motorL2 10

#define SERVO 11

// 常量
uint8_t expectedPWMR = 255;
const uint8_t delta = 30;  // 左右轮速度差
uint8_t expectedPWML = expectedPWMR - delta;
uint8_t strongPWM = expectedPWML;
uint8_t weakPWM = 25;

//采集到黑线时，灰度值较小
uint8_t id = 0;
uint8_t mode = 0;
uint8_t threshold[3] = { 190, 170, 240 };  //左中右的灰度阈值
int gray[3][2] = { 200 };
bool white[3] = { 0 };

// 颜色识别相关
// 卡片颜色
uint16_t red = 0, green = 0, blue = 0, sum = 0;
float r, g, b;
uint8_t color = 0, card_color = 0;
int color_time = 0;
int color_max_time = 4;
// 颜色阈值
uint16_t thresh_red = 6;    //3000
uint16_t thresh_green = 4;  //1600
uint16_t thresh_blue = 3;   //2500
// 颜色传感器
MH_TCS34725 tcs = MH_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);  // 50ms

// 初始化舵机
Timer2Servo my_servo;
// 舵机位置
uint8_t DOWN = 177, UP = 90;
uint8_t state = 90;
uint8_t flag = 0;
// 时间定时器
uint16_t upperBound = 1650;
uint16_t time_counter = 0;
uint16_t sensor_delay = 750;
uint8_t start = 0;

void setup() {
  Serial.begin(9600);
  pinMode(gray_right, INPUT);
  pinMode(gray_left, INPUT);
  pinMode(gray_mid, INPUT);
  pinMode(motorL1, OUTPUT);
  pinMode(motorL2, OUTPUT);
  pinMode(motorR1, OUTPUT);
  pinMode(motorR2, OUTPUT);
  pinMode(SERVO, OUTPUT);
}

void loop() {
  time_counter++;
  // pidMotorControl();
  // if(time_counter>1000)
  shoot();

  // stopMotor();

  // int res=colorDetect();
  // Serial.println(res);
  delay(50);
}
void stopMotor() {
  analogWrite(motorL1, 0);
  analogWrite(motorL2, 0);
  analogWrite(motorR1, 0);
  analogWrite(motorR2, 0);
}

void shoot() {
  //flag=0：正常情况；    flag=1，进入计时状态，连续检测到max个值进入2，否则回到0；
  //flag=2：记录卡片颜色，进入3；   flag=3：等待下降沿，若下降沿出现则进入4；
  //flag=4：若接下来任意瞬间比对成功，执行投球一次，进入5；   flag=5：不再进行颜色采集。
  if (flag < 5) {
    color = colorDetect();
    Serial.println(color_time);

    if (flag == 1 && color_time >= color_max_time)
      flag = 2;
    if (flag == 1 && color != 0)
      color_time++;
    if (flag == 1 && color == 0) {
      color_time = 0;
      flag = 0;
    }
    if (flag == 0 && color != 0)
      flag = 1;
    if (flag == 3 && color != card_color)
      flag = 4;
    if (flag == 2) {
      card_color = color;
      flag = 3;
    }

    if (flag == 4 && color == card_color) {
      my_servo.attach(SERVO);
      my_servo.write(UP);
      delay(sensor_delay);
      my_servo.write(DOWN);
      delay(sensor_delay);
      my_servo.detach();
      flag = 5;
    }
  }
}

void motorRun(int left, int right) {
  // left=0;right=0;
  // left=255;right=255;
  if (left < 0) left = 0;
  if (right < 0) right = 0;
  if (left >= 0) {
    left = left > 255 ? 255 : left;
    analogWrite(motorL1, left);
    analogWrite(motorL2, 0);
  } else if (left < 0) {
    left = left < -255 ? -255 : left;
    left = -left;
    analogWrite(motorL2, left);
    analogWrite(motorL1, 0);
  }
  if (right >= 0) {
    right = right > 255 ? 255 : right;
    analogWrite(motorR1, right);
    analogWrite(motorR2, 0);
  } else if (right < 0) {
    right = right < -255 ? -255 : right;
    right = -right;
    analogWrite(motorR2, right);
    analogWrite(motorR1, 0);
  }
}

uint8_t colorDetect() {
  tcs.getRGBC(&red, &green, &blue, &sum);
  // tcs.lock();
  // 提取颜色值
  // Serial.print(red);Serial.print("\t");
  // Serial.print(green);Serial.print("\t");
  // Serial.println(blue);
  // r = red;    r /= sum;
  // g = green;  g /= sum;
  // b = blue;   b /= sum;
  // 颜色转换
  // red = transform(r);
  // green = transform(g);
  // blue = transform(b);
  // Serial.print(r);Serial.print("\t");
  // Serial.print(g);Serial.print("\t");
  // Serial.println(b);
  if (red > green && red > blue && red > thresh_red) return 1;
  if (green > red && green > blue && green > thresh_green) return 2;
  if (blue > red && blue > green && blue > thresh_blue) return 3;
  return 0;
}

uint16_t transform(float a) {
  uint16_t ans = exp(10 * a) * 10;
  return ans;
}

void maintance() {
  uint8_t k1_big = 9, k1_small = 0;  //0~9
  uint8_t k2_big = 9, k2_small = 0;
  switch (mode) {
    case 0:
      motorRun(expectedPWML, expectedPWMR);
      break;
    case 3:
      motorRun(expectedPWML, expectedPWMR);
      break;
    case 2:
      motorRun(expectedPWML - k1_big * weakPWM, expectedPWMR - k1_small * weakPWM);
      break;
    case 1:
      motorRun(expectedPWML - k1_small * weakPWM, expectedPWMR - k1_big * weakPWM);
      break;
    case 5:
      motorRun(expectedPWML - k2_big * weakPWM, expectedPWMR - k2_small * weakPWM);
      break;
    case 4:
      motorRun(expectedPWML - k2_small * weakPWM, expectedPWMR - k2_big * weakPWM);
      break;
    default:
      break;
  }
}

void pidMotorControl() {
  uint8_t k1_big = 8, k1_small = 0;
  uint8_t k2_big = 9, k2_small = 0;
  
  gray[0][id] = analogRead(gray_left);   //左
  gray[1][id] = analogRead(gray_mid);    //中
  gray[2][id] = analogRead(gray_right);  //右
  //低通滤波
  for (int i = 0; i < 3; i++)
    white[i] = (0.7 * gray[i][id] + 0.3 * gray[i][!id]) > threshold[i] ? 1 : 0;  //为0则认为检测到黑色
  id = !id;
  bool left = white[0], mid = white[1], right = white[2];
  //电机控制
  //中黑
  if (mid == 0) {
    // 左右白
    if (left == 1 && right == 1) {
      mode = 0;
      motorRun(expectedPWML, expectedPWMR);
    }
    // 左白右黑
    else if (left == 1 && right == 0) {
      mode = 1;
      motorRun(expectedPWML - k1_small * weakPWM, expectedPWMR - k1_big * weakPWM);
    }
    // 左黑右白
    else if (left == 0 && right == 1) {
      mode = 2;
      motorRun(expectedPWML - k1_big * weakPWM, expectedPWMR - k1_small * weakPWM);
    }
    // 左右黑
    else {
      mode = 3;
      motorRun(expectedPWML, expectedPWMR);
    }
  }
  // 中白
  else {
    // 左右白
    if (left == 1 && right == 1) {
      maintance();
    }
    // 左白右黑
    else if (left == 1 && right == 0) {
      mode = 4;
      //  motorRun(expectedPWML, expectedPWMR - strongPWM);
      motorRun(expectedPWML - k2_small * weakPWM, expectedPWMR - k2_big * weakPWM);
    }
    // 左黑右白
    else if (left == 0 && right == 1) {
      mode = 5;
      // motorRun(expectedPWML - strongPWM, expectedPWMR);
      motorRun(expectedPWML - k2_big * weakPWM, expectedPWMR - k2_small * weakPWM);
    }
    // 左右黑
    else {
      maintance();
    }
  }
  // Serial.println(mode);
}
