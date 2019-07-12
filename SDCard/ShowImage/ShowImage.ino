#include <SPI.h>
#include <SD.h>

const int chipSelect = 31;

const int MAX_LINE = 200;
unsigned char buffer[MAX_LINE];
struct pixel {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};
struct image {
  int height;
  int width;
  pixel *val;
};

char * readHeaderLine(File fp) {
  // Check first character
  unsigned char next = fp.read();
  while (next == '#') { // We ignore these lines
    while (next != '\n') { // We can assume nice line feeds
      next = fp.read(); 
    }
    next = fp.read();
  }
  // Now we have a real line, read it
  int index = 0;
  while (next != '\n' && index < MAX_LINE) {
    buffer[index++] = next;
    next = fp.read();
  }
  buffer[index] = '\0';
  return buffer;
}

bool readPPM(char * name, image &im) {
  File dataFile = SD.open(name);
  if (!dataFile) return false;
  // Now read ppm file into memory.
  if (strncmp("P6", readHeaderLine(dataFile), 2)) {
    Serial1.println("Not a PPM file");
    return false;
  }    
  if (sscanf(readHeaderLine(dataFile), "%d %d", &im.width, &im.height) != 2) {
    Serial1.println("Missing width height line in the PPM file");
    return false;
  }
  unsigned maxValue = 0;
  if (sscanf(readHeaderLine(dataFile), "%d", &maxValue) != 1) {
    Serial1.println("Missing max pixel value line in the PPM file");
    return false;
  }
  if (im.height > 100 || im.width > 64 || maxValue != 255) {
    Serial1.println("Bad values in the PPM file");
    Serial1.println(im.width), Serial1.println(im.height), Serial1.println(maxValue);
    return false;
  }
  Serial1.println("Storing Image Pixels before Sending");
  int size = im.width * im.height;
  im.val = new pixel[size];
  int count = 0;
  while (size > count && dataFile.available()) {
    im.val[count].red = (dataFile.read() / 2) * 2;
    im.val[count].green = (dataFile.read() / 2) * 2;
    im.val[count].blue = (dataFile.read() / 2) * 2;
    count++;
  }
  dataFile.close();
  return count == size;
}

void setup()
{
  // Open Serial1 communications and wait for port to open:
  Serial1.begin(115200);
  Serial2.begin(460800);
  while (!Serial1) {
    ; // wait for Serial1 port to connect. Needed for Leonardo only
  }
  delay(2000);

  Serial1.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial1.println("Card failed, or not present");
    return;
  }
  Serial1.println("card initialized.");

  image im;
  if (readPPM("IMAGE.ppm", im)) {
    sendNext('{');
    sendNext('P');
    sendNext(im.width);
    sendNext(im.height);

    bool keepGoing = true;
    int count = 0;
    while (count < im.height * im.width && keepGoing) {
      keepGoing &= sendNext(im.val[count].red);
      keepGoing &= sendNext(im.val[count].green);
      keepGoing &= sendNext(im.val[count].blue);
      count++;
      Serial1.println(count);
    }
    sendNext('}');
  } else {
    Serial1.println("error processing IMAGE file");
  }
}

bool sendNext(unsigned char next) {
  Serial2.write(next);
  while (!Serial2.available()) {}
  return Serial2.read() == next;
}

void loop()
{
}
