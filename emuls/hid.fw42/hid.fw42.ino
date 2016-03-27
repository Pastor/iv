/**
  You must select Raw HID from the "Tools > USB Type" menu
  11 = Teensy 2.0
  13 = Teensy 3.0


0x10C4, 0x8468, 0xFF00, 0x0001

/arduino/hardware/teensy/avr/cores/usb_rawhid/usb_private.h 

#define VENDOR_ID               0x10C4
#define PRODUCT_ID              0x8468
#define RAWHID_USAGE_PAGE	0xFF00  
#define RAWHID_USAGE		0x0001  

**/
const int ledPin = 11;
byte buffer[64];
byte cmd[6];
elapsedMillis msUntilNextSend;
const byte hex_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
const byte chars[] = {
  '\r', ' ', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=',  '=',  '=',  '=',  '=',  '=',  '=',  '=', '=',
  '=',  '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=',  '=',  '=',  '=',  '=',  '\r', '\r', '\n',
  ' ', ' ', 'A',  'c', 'q', 'u', 'i', 's', 'i', 't', 'i', 'o', 'n', ' ', 'B', 'a',  's',  'e',  ' ',  'C',  'o',  'n',  't', 'r',
  'o',  'l', 'l', 'e', 'r', ':', ' ', 'v', 'e', 'r', ' ', '1', '.', '0',  '1',  ' ',  '\r', '\r', '\n', ' ', '=',  '=', '=',
  '=',  '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=',  '=',  '=',  '=',  '=',  '=',  '=',  '=', '=', '=',
  '=',  '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '\r', '\r', '\n', 0,    0
};

void setup() {
  int i = 0;

  for (i = 0; i < 7; i++) {
    pinMode(i, OUTPUT);
  }
  pinMode(ledPin, OUTPUT);


  i = 0;
  delay(10);
  while (chars[i] > 0) {
    delay(40);
    buffer[0] = 2;
    buffer[1] = chars[i];
    buffer[2] = chars[i + 1];
    RawHID.send(buffer, 100);
    i += 2;
  }
}

int from_hex(byte b[2]) {
  if (b[0] >= '0' && b[0] <= '9') {
    if (b[1] >= '0' && b[1] <= '9')
      return ((b[0] - '0') << 4 | b[1] - '0') & 255;
    return ((b[0] - '0') << 4 | (b[1] - 'A' + 10)) & 255;
  }
  if (b[1] >= '0' && b[1] <= '9')
    return ((b[0] - 'A' + 10) << 4 | b[1] - '0') & 255;
  return ((b[0] - 'A' + 10) << 4 | (b[1] - 'A' + 10)) & 255;
}

void to_hex(int g, byte b[2]) {
  b[0] = hex_chars[(g & 0xf0) >> 4];
  b[1] = hex_chars[(g & 0x0f) >> 0];
}

void loop() {
  int n;

  n = RawHID.recv(buffer, 0);
  if (n >= 2 && buffer[0] == 2 && buffer[1] == 6) {
    digitalWrite(ledPin, HIGH);
    cmd[0] = buffer[2];
    cmd[1] = buffer[3];
    cmd[2] = buffer[4];
    cmd[3] = buffer[5];
    cmd[4] = buffer[6];
    cmd[5] = buffer[7];
    if (cmd[0] == 'Q') {
      buffer[0] = 5;
      buffer[1] = cmd[0];
      buffer[2] = cmd[1];
      buffer[3] = cmd[2];
      buffer[4] = cmd[3];
      buffer[5] = ' ';
      RawHID.send(buffer, 100);
      delay(20);
      for (int i = 1; i <= 100; ++i) {
        int val = analogRead(i & 11);
        byte lo = lowByte(val) & 255;
        byte hi = highByte(val) & 255;
        buffer[0] = 5;
        to_hex(i, &buffer[1]);
        buffer[3] = 'E';
        buffer[4] = 'E';
        if (hi > 0 && hi < 7 && lo > 0 && lo < 7) {
          buffer[3] = ' ';
          buffer[4] = '0' + lo;
        }
        buffer[5] = ' ';
        RawHID.send(buffer, 100);
        delay(10);
      }
      buffer[0] = 5;
      buffer[1] = '\r';
      buffer[2] = ' ';
      buffer[3] = 'A';
      buffer[4] = 'L';
      buffer[5] = 'L';
      RawHID.send(buffer, 100);
      delay(20);
      buffer[0] = 2;
      buffer[1] = '\r';
      buffer[2] = '\n';
      RawHID.send(buffer, 100);
    } else  {
      buffer[0] = 5;
      buffer[1] = cmd[0];
      buffer[2] = cmd[1];
      buffer[3] = cmd[2];
      buffer[4] = '\r';
      buffer[5] = ' ';
      RawHID.send(buffer, 100);
    }
    delay(20);
    digitalWrite(ledPin, LOW);
  }
}
