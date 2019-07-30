#include <SPI.h>
#include <SD.h>

const int chipSelect = 31;

struct image_t {
  unsigned len;
  unsigned char width;
  unsigned char height;
  unsigned char *data;
  unsigned char id[7];
} image;

bool readMagic(File fp) {
  return (fp.read() == 'P'&& fp.read() == '6');
}

bool readFile(File fp) {
  image.len = fp.size() - 31;
  Serial1.print("The IMAGE.Z file has a payload of size: ");
  Serial1.println(image.len); 
  if (image.len >=  (1 << 12)) { // Data too long!
    return false; 
  }

  // Check first characters
  if (!readMagic(fp)) return false;
  Serial1.println("The magic bytes (P6) are correct");
  image.width = fp.read();
  image.height = fp.read();
  image.data = new unsigned char[image.len];
  Serial1.print("The Image has width:"); Serial1.print(image.width); Serial1.print(" and height:"); Serial1.println(image.height);
  fp.seek(31);
  if (fp.read(image.data, image.len) != image.len) {
    delete image.data;
    return false;
  }
  for (int index = 0 ; index < 40; index++) {
    Serial1.print(image.data[index]);
    Serial1.print(" ");
  }
  Serial1.println();
  Serial1.println("Managed to read the payload for sending over to the camera");
  return true;
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

  File imFile = SD.open("IMAGE.Z");
  if (readFile(imFile)) {
    sendNext('{');
    sendNext('P');
    sendNext((image.len >> 6) & 0x3F); // Max 12 bits length sent.
    sendNext(image.len & 0x3F);

    bool keepGoing = true;
    int count = 0;
    while (count < image.len && keepGoing) {
      keepGoing &= sendNext(image.data[count]);
      count++;
      Serial1.print(".");
      if (count %60 == 0) {
        Serial1.println();
      }
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
