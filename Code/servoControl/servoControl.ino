#include <Servo.h>
#include "math.h"

#define l1 9
#define l2 9
#define l3 4.5
#define l4 10.50979753
#define l5 2
#define pi 3.14159265359
#define epsilon 0.134970870
#define leftMax 161
#define rightMax 16
#define LOWERED  22
#define LIFTED 50
#define RIGHTX 4.5
#define LOWERY 10
#define UPPERY 19
#define LOWESTBOUNDY 7.5
#define SQUARESIZE 3
#define SERVOMDELAY 5000
#define CIRCLESTEPS 100

#define LOWERROW 11.5
#define MIDDLEROW 14.5
#define UPPERROW 17.5

Servo servoM, servoL, servoR;
double theta1, theta2, theta3, theta4;
double servoLsoll, servoRsoll;
double xlast, ylast; //at the end of a trajectory this variable save, the current x and y values are stored here
                     //stays consistent as long as  every function that moves the endeffector stores the values at the end
                     
int servoM_angle_last = LIFTED;
int decide = 1;
int count = 0;
int received = 0;
void (*functionPtr)(double,double); //Will be used in order to not use a case differentiation every time a move is made 


void setup()
{  
  Serial.begin(9600);
  
  servoM.attach(11);
  servoR.attach(10);
  servoL.attach(9);
  servoM_angle_last = LIFTED;
  functionPtr = &draw_cross;
  xlast = 0;
  ylast = 11;
  lift();
  delay(2);
  count = 0;
}

void loop ()
{  
   while( Serial.available() <= 0 ){}; //Waiting for the next command
   received = Serial.read();
   
   switch (received)                   //Execute command
     {
       case 'F': draw_field();                    break;
       case 'X': functionPtr = &draw_cross;       break;
       case 'O': functionPtr = &draw_circle;      break;
       case '1': (*functionPtr)(-3,LOWERROW);     break;
       case '2': (*functionPtr)(0,LOWERROW);      break;
       case '3': (*functionPtr)(3,LOWERROW);      break;
       case '4': (*functionPtr)(-3,MIDDLEROW);    break;
       case '5': (*functionPtr)(0,MIDDLEROW);     break;
       case '6': (*functionPtr)(3,MIDDLEROW);     break;
       case '7': (*functionPtr)(-3,UPPERROW);     break;
       case '8': (*functionPtr)(0,UPPERROW);      break;
       case '9': (*functionPtr)(3,UPPERROW);      break;
       case 'W': draw_smile();                    break;
       case 'L': draw_sulking();                  break;
       case 'D': draw_indifferent();              break;
     }
   go_to(0, LOWESTBOUNDY);
   received = 0;      //safety reset of received
   Serial.write('R');
   
}



//--------------------------------------------Functions for the Lifting servo servoM-------------------
void lower()
{
  int steps,i, difference, sign;
  if(servoM_angle_last == LOWERED) return;
  
  difference = LOWERED - servoM_angle_last; 
  sign = (difference > 0) - (difference < 0);
  steps = difference*sign;
  for(i = 0; i<= steps; i++)
  {
    servoM.write(servoM_angle_last + i*sign);
    delay(19);
  }
  servoM_angle_last = LOWERED;
}

void lift() //somehow not yet working
{
  int steps,i, difference, sign;
  if(servoM_angle_last == LIFTED) return;
  
  difference = LIFTED - servoM_angle_last; 
  sign = (difference > 0) - (difference < 0);
  steps = difference*sign;
  for(i = 0; i<= steps; i++)
  {
    servoM.write(servoM_angle_last + i*sign);
    delay(19);
  }
  servoM_angle_last = LIFTED;
}


//-------------------------------------------Help functions for setting the servos---------------------
double cosineLaw_angle(double a, double b, double c)
{
  return acos( (a*a + b*b - c*c)/(2*a*b));
}

double cosineLaw_side(double a, double b, double gamma) 
{
   return  sqrt(a*a + b*b - 2*a*b*cos(gamma));
}

void calculateServoMap(double x_prime, double y_prime, double * servoLsoll , double * servoRsoll)
{
  /*given the desired xy-coordinates, this function stores the values for the desired angels
  of the servo motors in the variables that are given by their pointers */
  double alpha, beta;
  double delta, phi;
  double theta1_prime, theta2_prime;
  double a, b, c, x, y;
  double d, f;
  
  //new part for code refinement
  d = sqrt( (x_prime + l3/2)*(x_prime + l3/2) + y_prime*y_prime);
  f = sqrt( x_prime*x_prime + y_prime*y_prime );
  
  theta1_prime = cosineLaw_angle(d, l1, l4);
  theta2_prime = cosineLaw_angle(d, l3/2, f);
  delta        = cosineLaw_angle(l4, l1, d);
  phi = delta - epsilon;
  
  a = cosineLaw_side(l1, l2, phi);
  theta1 = cosineLaw_angle(a, l1, l2);
  theta2 = theta1_prime + theta2_prime - theta1;
  
  y = a * sin(theta2);
  x = a * cos(theta2) - l3/2;
  
  alpha = atan2(y,x);
  
  c = sqrt(y*y + x*x); //c only defined by the distance of the endeffector towards the origin  
  
  b = cosineLaw_side(c, l3/2, alpha);//use cosine rule to get b
  /*
  //use sine rule to get theta2 and theta3, BUT ATTENTION: sine rule is ambiguous - case differentiation is necessary
  if( x > 1.125)
  {
    theta2 = asin(sin(beta) * c/a);
    theta3 = pi - asin(sin(alpha)* c/b);
  } 
  else if( x < -1.125)
  {
    theta2 = pi - asin(sin(beta) * c/a);
    theta3 = asin(sin(alpha)* c/b);
  } 
  else
  {
    theta2 = asin(sin(beta) * c/a);
    theta3 = asin(sin(alpha)* c/b);
  }
  */ //Sine rule is not optimal since it reequires case differentiation and makes the transition very unsmooth --> use cosine rule instead
  //still left here for reference
  
  theta3 = cosineLaw_angle(b, l3/2, c);
  theta4 = cosineLaw_angle(b, l1, l2);
  *servoLsoll = leftMax - (pi - (theta1+theta2) ) * 180/pi; 
  *servoRsoll = (pi - (theta3+theta4)) * 180/pi + rightMax;
}

void set_servos (double x, double y)
{
  double servoLsollint, servoRsollint;
  calculateServoMap(x,y, &servoLsollint, &servoRsollint);
  servoL.write(servoLsollint);
  servoR.write(servoRsollint);  
}

void go_to (double xto, double yto)
{
  int steps, i;
  double x_distance, y_distance, distance;
  x_distance = xto-xlast;
  y_distance = yto-ylast;
  distance = sqrt( x_distance*x_distance + y_distance*y_distance);
  
  //try always to do steps with the same step size, regardless of the distance
  //Desired stepsize should be 1 mm
  steps = (int) (10*distance);//10
  
  for( i = 0; i <= steps; i ++)
  {
   set_servos(xlast + (x_distance*i/steps), ylast + (y_distance*i/steps));
   delay(5);//5
  }
  set_servos(xto, yto);
  delay(100);
  xlast = xto;
  ylast = yto;
}

void draw_cross(double x_offset, double y_offset)
{
  lift();
  go_to( x_offset - 0.8 , y_offset +  0.8);
  lower();
  go_to( x_offset +  0.8 , y_offset -  0.8);
  lift();
  go_to( x_offset +  0.8 , y_offset +  0.8);
  lower();
  go_to( x_offset -  0.8 , y_offset -  0.8);
  lift();
}

void draw_circle (double x_offset, double y_offset)
{
  int  i;
  lift();
  go_to( x_offset , y_offset + 0.8);
  lower();
  
  for(i = 0; i< CIRCLESTEPS; i++)
  {
    //go_to( x_offset + 1.125 * sin(2*pi*i/CIRCLESTEPS), y_offset + 1.125 *cos(2*pi*i/CIRCLESTEPS));
    set_servos(x_offset + 0.8 * sin(2*pi*i/CIRCLESTEPS), y_offset + 0.8 *cos(2*pi*i/CIRCLESTEPS));
    delay(5);
  }
  go_to( x_offset , y_offset + 0.8);
  lift();
}

void draw_field ()
{
  lift();
  go_to ( RIGHTX - SQUARESIZE,  LOWERY);
  lower();
  go_to ( RIGHTX - SQUARESIZE, UPPERY);
  lift();
  go_to ( RIGHTX,  UPPERY -SQUARESIZE);
  lower();
  go_to ( -RIGHTX, UPPERY- SQUARESIZE);
  
  lift();
  go_to ( -RIGHTX + SQUARESIZE, UPPERY);
  lower();
  go_to ( -RIGHTX + SQUARESIZE, LOWERY);
  lift();
  go_to ( -RIGHTX,  LOWERY +SQUARESIZE);
  lower();
  go_to (  RIGHTX, LOWERY + SQUARESIZE);
  lift();
}

void draw_next( double x_offset, double y_offset)
{
 if (decide == 1) 
  {
    draw_cross(x_offset, y_offset);
    decide = 0;
  } else
  {
    draw_circle(x_offset, y_offset);
    decide = 1;
  }
  lift();
}

void driveToMontagePosition() //DO NOT USE WHILE ROBOT IS ASSEMBLED!!!!!!!!!!!!!!!
{
  servoM.write(90);
}

void wipe() //wiping with the robot does not wirk
{
  lift();
  go_to(6,16);
  lower();
  go_to( RIGHTX, UPPERY);
  go_to( -RIGHTX, UPPERY);
  go_to( -RIGHTX, UPPERY-0.5*SQUARESIZE);
  go_to( RIGHTX, UPPERY-0.5*SQUARESIZE);
  go_to( RIGHTX, UPPERY-SQUARESIZE);
  go_to( -RIGHTX, UPPERY-SQUARESIZE);
  go_to( -RIGHTX, UPPERY-1.5*SQUARESIZE);
  go_to( RIGHTX, UPPERY-1.5*SQUARESIZE);
  go_to( RIGHTX, UPPERY-2*SQUARESIZE);
  go_to( -RIGHTX, UPPERY-2*SQUARESIZE);
  go_to( -RIGHTX, UPPERY-2.5*SQUARESIZE);
  go_to( RIGHTX, UPPERY-2.5*SQUARESIZE);
  go_to(6,16);
  lift();
}

void draw_big_smile()
{
  int i;
  lift();
  go_to ( RIGHTX - SQUARESIZE,  LOWERY);
  lower();
  go_to ( RIGHTX - SQUARESIZE,  LOWERY + SQUARESIZE);
  
  lift();
  go_to ( -RIGHTX + SQUARESIZE,  LOWERY);
  lower();
  go_to ( -RIGHTX + SQUARESIZE,  LOWERY + SQUARESIZE);
  lift();
  
  go_to( -1.5*SQUARESIZE , 11.5);
  
  lower();
  for(i = 0; i< CIRCLESTEPS; i++)
  {
    //go_to( x_offset + 1.125 * sin(2*pi*i/CIRCLESTEPS), y_offset + 1.125 *cos(2*pi*i/CIRCLESTEPS));
    set_servos(0 + 1.5*SQUARESIZE* sin(pi*i/CIRCLESTEPS-pi/2), 11.5 + 1.5*SQUARESIZE*cos(pi*i/CIRCLESTEPS-pi/2));
    delay(5);
  }
  xlast = -1.5*SQUARESIZE;
  ylast = 11.5;
  lift();
  
}

void draw_smile()
{
  int i;
  lift();
  go_to ( 7.75,  9);
  lower();
  go_to ( 7.75,  10.5);
  lift();
  go_to ( 6.25,  9);
  lower();
  go_to ( 6.25,  10.5);
  lift();
  go_to( 5 , 11);
  lower();
  for(i = 0; i< CIRCLESTEPS; i++)
  {
    //go_to( x_offset + 1.125 * sin(2*pi*i/CIRCLESTEPS), y_offset + 1.125 *cos(2*pi*i/CIRCLESTEPS));
    set_servos(7 + 2* sin(pi*i/CIRCLESTEPS-pi/2), 11 + 2*cos(pi*i/CIRCLESTEPS-pi/2));
    delay(5);
  }
  xlast = 9;
  ylast = 11;
  lift();
}

void draw_sulking()
{
  int i;
  lift();
  go_to ( 7.75,  9);
  lower();
  go_to ( 7.75,  10.5);
  lift();
  go_to ( 6.25,  9);
  lower();
  go_to ( 6.25,  10.5);
  lift();
  go_to( 9 , 13);
  lower();
  for(i = 0; i< CIRCLESTEPS; i++)
  {
    //go_to( x_offset + 1.125 * sin(2*pi*i/CIRCLESTEPS), y_offset + 1.125 *cos(2*pi*i/CIRCLESTEPS));
    set_servos(7 + 2* sin(pi*i/CIRCLESTEPS+pi/2), 13 + 2*cos(pi*i/CIRCLESTEPS+pi/2));
    delay(5);
  }
  xlast = 5;
  ylast = 13;
  lift();
}

void draw_indifferent()
{
  int i;
  lift();
  go_to ( 7.75,  9);
  lower();
  go_to ( 7.75,  10.5);
  
  lift();
  go_to ( 6.25,  9);
  lower();
  go_to ( 6.25,  10.5);
  lift();
  go_to( 9 , 11);
  lower();
  go_to(5,11);
  lift();
}

