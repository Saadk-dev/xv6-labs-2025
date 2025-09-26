#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data);

int
main(int argc, char *argv[])
{
  if(argc == 1){
    printf("Example 1:\n");
    int a[2] = { 61810, 2025 };
    memdump("ii", (char*) a);
    
    printf("Example 2:\n");
    memdump("S", "a string");
    
    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *) &s);

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;
    
    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");
    
    printf("Example 4:\n");
    memdump("pihcS", (char*) &example);
    
    printf("Example 5:\n");
    memdump("sccccc", (char*) &example);
  } else if(argc == 2){
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while(n < sizeof(data)){
      int nn = read(0, data + n, sizeof(data) - n);
      if(nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

void
memdump(char *fmt, char *data)
{
  // Walk 'data' as raw bytes
  unsigned char *p = (unsigned char *)data;

  for (char *f = fmt; *f; f++) {
    switch (*f) {

      case 'i': {                      // next 4 bytes: 32-bit int (decimal)
        unsigned int v;
        memmove(&v, p, 4);
        p += 4;
        printf("%d\n", (int)v);
        break;
      }

      case 'p': {                      // next 8 bytes: 64-bit int (hex, no 0x)
        unsigned long long v;
        memmove(&v, p, 8);
        p += 8;

        // print v in lowercase hex without leading zeros
        if (v == 0) {
          printf("0\n");
        } else {
          char buf[17];                // 16 hex nibbles + NUL
          int i = 16;
          buf[i] = 0;
          while (v && i > 0) {
            int nib = v & 0xF;
            buf[--i] = (nib < 10) ? ('0' + nib) : ('a' + nib - 10);
            v >>= 4;
          }
          printf("%s\n", &buf[i]);
        }
        break;
      }

      case 'h': {                      // next 2 bytes: 16-bit int (decimal)
        unsigned short v;
        memmove(&v, p, 2);
        p += 2;
        printf("%d\n", (int)v);
        break;
      }

      case 'c': {                      // next 1 byte: ASCII char
        unsigned char v = *p++;
        printf("%c\n", (char)v);
        break;
      }

      case 's': {                      // next 8 bytes: pointer to C-string
        unsigned long long addr;
        memmove(&addr, p, 8);
        p += 8;
        char *sp = (char *)addr;
        printf("%s\n", sp);
        break;
      }

      case 'S': {                      // rest of data is a C-string
        char *sp = (char *)p;
        printf("%s\n", sp);
        while (*p) p++;                // consume bytes up to NUL
        p++;                           // skip the NUL
        break;
      }

      default:
        // ignore unknown format chars
        break;
    }
  }
}


