import processing.serial.*;

Serial myPort;              

int redVal = 0;
int greenVal = 0; 
int blueVal = 0;

long lastSendTime = 0;


void setup() {
  size(450, 255);


  // Print a list of the serial ports, for debugging purposes:
  printArray(Serial.list());

  String portName = Serial.list()[5];
  myPort = new Serial(this, portName, 9600);
}

void draw() {
  background(0);


  noStroke();
  //red
  fill(255, 0, 0);
  rect(0, height, width/3, -redVal);
  //green
  fill(0, 255, 0);
  rect(width/3, height, width/3, -greenVal);
  //blue
  fill(0, 0, 255);
  rect(width * 2/3, height, width/3, -blueVal);
  

  float boxHeight = 27;
  fill(0);
  rect(0, height - boxHeight, width/6, boxHeight);
  rect(width/3, height - boxHeight, width/6, boxHeight);
  rect(width * 2/3, height - boxHeight, width/6, boxHeight);

  fill(255);
  textSize(20);
  text(str(redVal), 10, height - 5);
  text(str(greenVal), width/3 + 10, height - 5);
  text(str(blueVal), width * 2/3 + 10, height - 5);
  
  //dividing lines
  stroke(255);
  line(width/3, 0, width/3, height);
  line(width * 2/3, 0, width * 2/3, height);
  
  
  if ( millis() - lastSendTime > 50) {
    String sendString = "c" + str(redVal) + "," + str(greenVal) + "," + str(blueVal) + "\n";
    myPort.write(sendString);
    println( sendString );
    
    lastSendTime = millis();
  }
  
}

void mouseDragged() {

  int val = int(constrain(map(mouseY, 0, height, 255, 0), 0, 255));


  if (mouseX < width/3) {
    redVal = val;
  } else if ( mouseX < width * 2/3) {
    greenVal = val;
  } else {
    blueVal = val;
  }
}



void keyPressed() {

  if (key == 'b') {
    myPort.write("b0");
  }

  if (key == 'c') {
    myPort.write("c100,100,100");
  }
}