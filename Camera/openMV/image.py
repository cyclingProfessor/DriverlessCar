class Image(list):
    def __init__(self, a, b, c):
        list.__init__(self)
        for i in range(a * b):
          self.append(i)
        self.w = a
        self.h = b
    def width(self):
        return self.w
    def height(self):
        return self.h
    def __getitem__(self,index):
      return super(Image, self).__getitem__(index)
