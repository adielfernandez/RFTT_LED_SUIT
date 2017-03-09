Only works if the "Suit_Test" program is on the teensy!

If it doesnt work, check to make sure Processing is connecting to the correct teensy
by looking at the console log, seeing which number is the teensy then putting that
number in the "String portName = Serial.list()[5];" line in setup.