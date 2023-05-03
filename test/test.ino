#include <Servo.h>
#include <Wire.h>         //调用IIC库函数
#include "MH_TCS34725.h"  //调用颜色识别传感器库函数
#ifdef __AVR__
//#include <avr/power.h>
#endif
#define gray_right A4  //right
#define gray_left A3  //left
#define gray_mid A0  //middle
#define motorR1 5
#define motorR2 6
#define motorL1 9
#define motorL2 10
#define motor3L 8
#define motor3L2 3
#define motor3R 7
#define motor3R2 12
#define SERVO 11

// 常量
const uint8_t SPEED = 250;
const uint8_t theta = 0;  // 左右轮速度差
uint8_t deltaPWM = 250;
uint8_t weakPWM = 25;
const uint8_t time_delay = 10;

//初始化目标速度
uint8_t expectedPWML = 250;
uint8_t expectedPWMR = expectedPWML;
int beginSpeed = 50;
int stopSpeed = 130;

//采集到黑线时，传感器为低电平
uint8_t id = 0;
uint8_t mode = 0;
uint8_t threshold[3] = {200,200,250};//左中右的阈值

int gray[3][2] = {200, 200, 200, 200, 200, 200};
bool white[3] = {0, 0, 0};
// 颜色识别相关
// 卡片颜色
uint16_t red, green, blue, sum;
float r, g, b;
uint8_t color = 0, card_color = 0;
// 颜色阈值
uint16_t thresh_red = 3000;//3000
uint16_t thresh_green = 0;//1600
uint16_t thresh_blue = 0;//2500
// 颜色传感器
MH_TCS34725 tcs = MH_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);  // 50ms

// 初始化舵机
Servo my_servo;
// 舵机位置
uint8_t DOWN = 0, UP = 180, STOP = 90;
uint8_t state=90;
// 时间定时器
uint16_t upperBound = 1650;
uint16_t time_counter = 0;
uint16_t sensor_delay = 750;
uint8_t start = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(gray_right, INPUT);
  pinMode(gray_left, INPUT);
  pinMode(gray_mid, INPUT);
  pinMode(motorL1, OUTPUT);
  pinMode(motorL2, OUTPUT);
  pinMode(motorR1, OUTPUT);
  pinMode(motorR2, OUTPUT);
  pinMode(SERVO, OUTPUT);
  expectedPWML -= beginSpeed;
  expectedPWMR -= beginSpeed;
}

void loop() {
  // if (time_counter == upperBound - 1000) {
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

  //------gsk test-------

  uint8_t input = Serial.read();
  if(input == 'a')
    start = 1;
  if(input == 's')
    start = 0;
  // Serial.println(start);

  if(start)
  {
    // my_servo.attach(SERVO);
    // my_servo.write(UP);
    // delay(sensor_delay);
    // my_servo.write(DOWN);
    // delay(sensor_delay);
    // my_servo.detach();

    // analogWrite(motorR1, 227);//右大轮，值越大越快
    // analogWrite(motorL1, 227);//左大轮，值越大越快
    // analogWrite(8, 0);//中间的小轮，值越小越快
    // analogWrite(7, 0);
    pidMotorControl();
    delay(20);
    
  }
  else
  {
    analogWrite(motorL1, 0);analogWrite(motorL2, 0);
    analogWrite(motorR1, 0);analogWrite(motorR2, 0);
    analogWrite(motor3L, 255);analogWrite(motor3L2, 255);
    analogWrite(motor3R, 255);analogWrite(motor3R2, 255);

    // gray[0][id] = analogRead(gray_left);
    // gray[1][id] = analogRead(gray_mid);
    // gray[2][id] = analogRead(gray_right);
    // Serial.print(gray[0][id]);Serial.print('\t');
    // Serial.print(gray[1][id]);Serial.print('\t');
    // Serial.println(gray[2][id]);
    // id = !id;
    delay(100);
  }
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
  // left=-187;right=187;
  if(left>=0)
  {
    left = left > 255 ? 255 : left;
    analogWrite(motorL1, left);  
    analogWrite(motorL2, 0);
    analogWrite(motor3L, 255-left);
    analogWrite(motor3L2, 255);
    if(right>=0)
    {  
      right = right > 255 ? 255 : right;
      analogWrite(motorR1, right);
      analogWrite(motorR2, 0);
      analogWrite(motor3R, 255-right);
      analogWrite(motor3R2, 255);
    }
    else if(right<0)
    {
      right = right < -255 ? -255 : right;
      right = -right;
      analogWrite(motorR2, right);
      analogWrite(motorR1, 0);
      analogWrite(motor3R2, 255-right);
      analogWrite(motor3R, 255);
    }
  }
  else if(left<0)
  {
    left = left < -255 ? -255 : left;
    left = -left;
    analogWrite(motorL2, left);
    analogWrite(motorL1, 0);
    analogWrite(motor3L2, 255-left);
    analogWrite(motor3L, 255);
    if(right>=0)
    {  
      right = right > 255 ? 255 : right;
      analogWrite(motorR1, right);
      analogWrite(motorR2, 0);
      analogWrite(motor3R, 255-right);
      analogWrite(motor3R2, 255);
    }
    else if(right<0)
    {
      right = right < -255 ? -255 : right;
      right = -right;
      analogWrite(motorR2, right);
      analogWrite(motorR1, 0);
      analogWrite(motor3R2, 255-right);
      analogWrite(motor3R, 255);
    }
  }


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

void maintance(){
  switch (mode) {
    case 0:
      motorRun(expectedPWML, expectedPWMR);
      break;
    case 3:
      motorRun(expectedPWML, expectedPWMR);
      break;
    case 2:
      motorRun(expectedPWML - 8 * weakPWM, expectedPWMR - 1*weakPWM);
      break;
    case 1:
      motorRun(expectedPWML - 1*weakPWM, expectedPWMR - 8 * weakPWM);
      break;
    case 5:
      motorRun(expectedPWML - deltaPWM-150, expectedPWMR-70);
      break;
    case 4:
      motorRun(expectedPWML-70, expectedPWMR - deltaPWM-150);
      break;
    default:
      break;
  }
}

void pidMotorControl() {
  gray[0][id] = analogRead(gray_left);//左
  gray[1][id] = analogRead(gray_mid);//中
  gray[2][id] = analogRead(gray_right);//右
  //低通滤波
  for(int i=0;i<3;i++)
    white[i] = (0.6 * gray[i][id] + 0.4 * gray[i][!id]) > threshold[i] ? 1 : 0; //为0则认为检测到黑色
  id = !id;
  int left=white[0], mid=white[1], right=white[2];
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
      motorRun(expectedPWML - weakPWM, expectedPWMR - 8 * weakPWM);
    }
    // 左黑右白
    else if (left == 0 && right == 1) {
      mode = 2;
      motorRun(expectedPWML - 8 * weakPWM, expectedPWMR - weakPWM);
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
      motorRun(expectedPWML - 70, expectedPWMR - deltaPWM -150);
    } 
    // 左黑右白
    else if (left == 0 && right == 1) {
      mode = 5;
      motorRun(expectedPWML - deltaPWM - 150, expectedPWMR - 70);
    } 
    // 左右黑
    else {
      maintance();
    }
  }
  // Serial.println(mode);
}
