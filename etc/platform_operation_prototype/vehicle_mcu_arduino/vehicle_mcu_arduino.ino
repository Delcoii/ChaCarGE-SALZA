/*
 * 
 * no Serial print while using ROS communication!!!
*/

// #include <ros.h>
// #include <std_msgs/Float32.h>

// for ROS publish
#define LOOP_MS                 16

// pulse width detection (10ms ~ 20 ms)
#define PULSE_ERROR_MAX_US      2500
#define PULSE_ERROR_MIN_US      500
#define DETECTION_ERR           -999

// pin - motor (left, right, steer)
#define STEERING_PULSE_PIN      18
#define ACCEL_PULSE_PIN         19
#define LEFT_IN1_PIN            4
#define LEFT_IN2_PIN            5
#define RIGHT_IN1_PIN           6
#define RIGHT_IN2_PIN           7
#define STEER_IN1_PIN           8
#define STEER_IN2_PIN           9 
#define PIN_10                  10    // for ENA or ENB pin (not used)
#define PIN_11                  11    // for ENA or ENB pin (not used)
#define PIN_12                  12    // for ENA or ENB pin (not used)

// potentiometer
#define STEER_DETECT_PIN        A0

// changing mode by pulse (CH3)
#define MODE_PULSE_PIN          21
#define MANUAL_MODE             1200    // for pretty plotting
#define AUTO_MODE               1500    // for pretty plotting     
#define BRAKE_MODE              1800    // for pretty plotting

#define CH3_PULSE_PIN           20      // not used

// for 0.0 ~ 1.0 value
#define ACCEL_OFFSET            0.0
#define STEER_OFFSET            0.0
#define SIGNAL_THRESHOLD        0.1     // motor launching value

// PID control       
#define STEER_DETECT_MAX        410     // potentiometer value when steer is right end
#define STEER_DETECT_MIN        749     // potentiometer value when steer is right end
#define MAX_STEER_TIRE_DEG      20.09
#define KP                      0.2
#define KI                      0
#define KD                      0

// for enabling 12V relay
#define RELAY_SIGNAL_PIN        23      // connected to IN1 in relay
#define HV_SWITCH_PIN           53
#define RELAY_DEBOUNCE_MS       50


volatile long g_steering_edge_now_us = DETECTION_ERR;
volatile long g_steering_edge_before_us = DETECTION_ERR;
volatile long g_steering_us = DETECTION_ERR;

volatile long g_accel_edge_now_us = DETECTION_ERR;
volatile long g_accel_edge_before_us = DETECTION_ERR;
volatile long g_accel_us = DETECTION_ERR;

volatile long g_mode_edge_now_us = DETECTION_ERR;
volatile long g_mode_edge_before_us = DETECTION_ERR;
volatile long g_mode_us = DETECTION_ERR;



double PID(double ref, double sense, double dt_us) {
  static double prev_err = 0.0;
  double err = ref - sense;
  double P = err * KP;
  double I = err * dt_us * KI;
  double D = ((err - prev_err) / dt_us) * KD;

  prev_err = err;
  return P+I+D;
}


void SteeringPulseInt() {
    g_steering_edge_now_us = micros();

    g_steering_us = g_steering_edge_now_us - g_steering_edge_before_us;
    g_steering_edge_before_us = g_steering_edge_now_us;
}

void AccelPulseInt() {
    g_accel_edge_now_us = micros();

    g_accel_us = g_accel_edge_now_us - g_accel_edge_before_us;
    g_accel_edge_before_us = g_accel_edge_now_us;
}

void ModePulseInt() {
    g_mode_edge_now_us = micros();
    
    g_mode_us = g_mode_edge_now_us - g_mode_edge_before_us;
    g_mode_edge_before_us = g_mode_edge_now_us;
}


float Mapping(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void StopMotor() {
  analogWrite(LEFT_IN1_PIN, 0);
  analogWrite(LEFT_IN2_PIN, 0);
  analogWrite(RIGHT_IN1_PIN, 0);
  analogWrite(RIGHT_IN2_PIN, 0);
  analogWrite(STEER_IN1_PIN, 0);
  analogWrite(STEER_IN2_PIN, 0);
}


// input throttle 0.0 ~ 1.0
void MoveForward(double throttle) {
  if (throttle > 1.0) {
    throttle = 1.0;
  } else if (throttle < 0.0) {
    throttle = 0.0;
  }
  int in = (int)(Mapping(throttle, 0.0, 1.0, 0.0, 255.0));
  if (abs(throttle) < SIGNAL_THRESHOLD) {
    in = 0; 
  }
  
  analogWrite(LEFT_IN1_PIN, 0);
  analogWrite(LEFT_IN2_PIN, in);
  analogWrite(RIGHT_IN1_PIN, in);
  analogWrite(RIGHT_IN2_PIN, 0);
}

// input backward 0.0 ~ 1.0
void MoveBackward(double throttle) {
  if (throttle > 1.0) {
    throttle = 1.0;
  } else if (throttle < 0.0) {
    throttle = 0.0;
  }
  int in = (int)(Mapping(throttle, 0.0, 1.0, 0.0, 255.0));
  if (abs(throttle) < SIGNAL_THRESHOLD) {
    in = 0; 
  }
  

  analogWrite(LEFT_IN1_PIN, in);
  analogWrite(LEFT_IN2_PIN, 0);
  analogWrite(RIGHT_IN1_PIN, 0);
  analogWrite(RIGHT_IN2_PIN, in);
}


// input steer -1.0 ~ 1.0
void Steer(double throttle) {
  if (throttle > 1.0) {
    throttle = 1.0;
  } else if (throttle < -1.0) {
    throttle = -1.0;
  }
  int in = (int)(Mapping(throttle, -1.0, 1.0, -255.0, 255.0));
  if (abs(throttle) < SIGNAL_THRESHOLD) {
    in = 0; 
  }
  

  if (in > 0.0) {
    analogWrite(STEER_IN1_PIN, in);
    analogWrite(STEER_IN2_PIN, 0);
  }
  else {
    in *= -1.0;
    analogWrite(STEER_IN1_PIN, 0);
    analogWrite(STEER_IN2_PIN, in);
  }
}


/* // ros settings
ros::NodeHandle nh;
float throttle_msg_ = 0.0;   // -1.0 ~ 1.0 required
float steer_msg_ = 0.0;      // -1.0 ~ 1.0 required
std_msgs::Float32 o_steer_deg;
std_msgs::Float32 o_forward;
std_msgs::Float32 o_backward;
// std_msgs::Float32 o_potentiometer;

ros::Subscriber<std_msgs::Float32> s_throttle("/car/throttle", ThrottleCb);
ros::Subscriber<std_msgs::Float32> s_steer("/car/steering", SteerCb);
ros::Publisher p_steer_deg("/car/steer_out_deg", &o_steer_deg);
ros::Publisher p_forward("/car/forward_out", &o_forward);
ros::Publisher p_backward("/car/backward_out", &o_backward);
// ros::Publisher p_potentio("/car/potentiometer", &o_potentiometer);


void ThrottleCb( const std_msgs::Float32& msg) {
  if (msg.data >= -1.0 && msg.data <= 1.0) {
    throttle_msg_ = msg.data;
  } else if (msg.data > 1.0) {
    throttle_msg_ = 1.0;
  } else if (msg.data < -1.0) {
    throttle_msg_ = -1.0;
  }
}

// value to be -19.85 ~ 19.85
void SteerCb( const std_msgs::Float32& msg) {
  steer_msg_ = msg.data;
}
*/



void setup() {
  Serial.begin(9600);
  
  pinMode(STEERING_PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(STEERING_PULSE_PIN), SteeringPulseInt, CHANGE);
  pinMode(ACCEL_PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ACCEL_PULSE_PIN), AccelPulseInt, CHANGE);
  pinMode(MODE_PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MODE_PULSE_PIN), ModePulseInt, CHANGE);
  pinMode(STEER_DETECT_PIN, INPUT_PULLUP);
  
  pinMode(LEFT_IN1_PIN, OUTPUT);
  pinMode(LEFT_IN2_PIN, OUTPUT);
  pinMode(RIGHT_IN1_PIN, OUTPUT);
  pinMode(RIGHT_IN2_PIN, OUTPUT);
  pinMode(STEER_IN1_PIN, OUTPUT);
  pinMode(STEER_IN2_PIN, OUTPUT);

  pinMode(RELAY_SIGNAL_PIN, OUTPUT);
  pinMode(HV_SWITCH_PIN, INPUT_PULLUP);

  digitalWrite(PIN_10, HIGH);     // NOT USED
  digitalWrite(PIN_11, HIGH);     // NOT USED
  digitalWrite(PIN_12, HIGH);     // NOT USED
  digitalWrite(RELAY_SIGNAL_PIN, LOW);


  StopMotor();

  /*
  nh.initNode();
  nh.advertise(p_steer_deg);
  nh.advertise(p_forward);
  nh.advertise(p_backward);

  nh.subscribe(s_throttle);
  nh.subscribe(s_steer);
  */
}

void loop() {
  static int prev_t_us = 0;
  int now_us = micros();

  /*************** calculating pulse width ***************/
  static int steering_val;
  static int accel_val;
  static int mode_val = BRAKE_MODE;

  cli();
  if ((g_steering_us > PULSE_ERROR_MIN_US) && (g_steering_us < PULSE_ERROR_MAX_US)) {
      steering_val = g_steering_us;
  }
  if ((g_accel_us > PULSE_ERROR_MIN_US) && (g_accel_us < PULSE_ERROR_MAX_US)) {
      accel_val = g_accel_us;
  }

  if ((g_mode_us > PULSE_ERROR_MIN_US) && (g_mode_us < PULSE_ERROR_MAX_US)) {
      if ((g_mode_us >= 800) && (g_mode_us <= 1200)) {
          mode_val = MANUAL_MODE;
      }
      else if ((g_mode_us > 1200) && g_mode_us <= 1700) {
          mode_val = AUTO_MODE;
      }
      else {
          mode_val = BRAKE_MODE;
      }
  }
  sei();
  /*************** calculating pulse width end ************/


  /*************** enable 12v by using relay ************/
  static int prev_hv_sw_state = HIGH;
  static int now_hv_sw_state = HIGH;
  static unsigned long prev_hv_sw_change_ms = millis();


  // read pin & save change time
  now_hv_sw_state = digitalRead(HV_SWITCH_PIN);
  if (prev_hv_sw_state != now_hv_sw_state) {
    prev_hv_sw_change_ms = millis();
  }

  // sw state is same during relay debounce time
  if ((millis() - prev_hv_sw_change_ms) > RELAY_DEBOUNCE_MS) {
    // TODO : not write pin val when sw state is stabled
    if (now_hv_sw_state == HIGH) {
      digitalWrite(RELAY_SIGNAL_PIN, HIGH); // why??
    }
    else {
      digitalWrite(RELAY_SIGNAL_PIN, LOW);
    }
  }
  prev_hv_sw_state = now_hv_sw_state;
  /*************** enable 12v by using relay end ************/


  /*************** changing pulse width to motor input ************/
  float throttle_input = Mapping(accel_val, 1000.0, 2000.0, -1.0, 1.0);
  float steer_input = Mapping(steering_val, 1000.0, 2000.0, -1.0, 1.0);
  if (throttle_input < -1.5 || throttle_input > 1.5
      || steer_input < -1.5 || steer_input > 1.5)  {
      throttle_input = 0.0;
      steer_input = 0.0;
  }  

  throttle_input += ACCEL_OFFSET;
  steer_input += STEER_OFFSET;
  Serial.print("throttle:");
  Serial.print(throttle_input);
  Serial.print("\tsteer:");
  Serial.print(steer_input);
  Serial.print("\tmode:");
  Serial.println(mode_val);
  
  
  // changing steer value to steering tire degree
  double ref_steer_tire_deg = Mapping(steer_input, -1.0, 1.0, -MAX_STEER_TIRE_DEG, MAX_STEER_TIRE_DEG);
  ref_steer_tire_deg *= -1.0;   // TODO : change by macro
  int potentio_val = analogRead(STEER_DETECT_PIN);
  double steer_tire_deg = Mapping(potentio_val, STEER_DETECT_MIN, STEER_DETECT_MAX, MAX_STEER_TIRE_DEG, -MAX_STEER_TIRE_DEG);
  
  unsigned int dt_us;
  double pid_return;
  /************ changing pulse width to motor input end ***********/




  /*************** operating motors ************/
  if (mode_val == BRAKE_MODE) {
    StopMotor();
  }
  else if (mode_val == MANUAL_MODE) {
    if (throttle_input > 0.0) {
      MoveForward(throttle_input);
      // o_forward.data = throttle_input;
      // o_backward.data = 0.0;
    }
    else {
      throttle_input *= -1.0;
      MoveBackward(throttle_input);
      // o_forward.data = 0.0;
      // o_backward.data = throttle_input;
    }
  
    dt_us = now_us - prev_t_us;
    pid_return = PID(ref_steer_tire_deg, steer_tire_deg, dt_us);
    // Serial.println(pid_return);
    if (pid_return > 1.0) {
      pid_return = 1.0;
    } else if (pid_return < -1.0) {
      pid_return = -1.0;
    }
    Steer(pid_return);
    // o_steer_deg.data = deg;
    // o_forward.data = throttle_input;
    // o_potentiometer.data = potentio_val;
    /*************** operating motors end ************/
  }

  /*
  else if (mode_val == AUTO_MODE) {
    if (throttle_msg_ > 0.0) {
      MoveForward(throttle_msg_);
      o_forward.data = throttle_msg_;
      o_backward.data = 0.0;
    }
    else {
      throttle_msg_ *= -1.0;
      MoveBackward(throttle_msg_);
      o_forward.data = 0.0;
      o_backward.data = throttle_msg_;
    }

    pid_return = PID(steer_msg_, deg, dt);
    if (pid_return > 1.0) {
      pid_return = 1.0;
    } else if (pid_return < -1.0) {
      pid_return = -1.0;
    }
    Steer(pid_return);
    o_steer_deg.data = steer_tire_deg;
    o_forward.data = throttle_msg_;
    // o_potentiometer.data = potentio_val;
  }
  */


  // publish by equal freq
  static int before_ms = 0;
  int now_ms = millis();
  if ((now_ms - before_ms) >= LOOP_MS) {
    before_ms = now_ms;

    // p_steer_deg.publish(&o_steer_deg);
    // p_forward.publish(&o_forward);
    // p_backward.publish(&o_backward);
    // p_potentio.publish(&o_potentiometer);
  }

  // Serial.print("ref:");
  // Serial.print(ref_steer_deg);
  // Serial.print("\t,");
  // Serial.print("deg:");
  // Serial.println(deg);
  // Serial.print("pid out:");
  // Serial.println(pid_return);

  // nh.spinOnce();

  prev_t_us = now_us;

}