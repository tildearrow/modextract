/**
 * modextract - extract samples from .mod files
 * Copyright (C) 2023 tildearrow
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "sndfile.h"

#ifdef _WIN32
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

FILE* f;
char str[4096];

int sampleRates[8]={
  8363,
  8363,
  8363,
  8363,
  8363,
  8363,
  8363,
  8363
};

struct MODFile {
  char modName[20];
  struct {
    char name[22];
    unsigned short len;
    unsigned char fine, vol;
    unsigned short loopStart, loopLen;
  } ins[31];
  unsigned char ordersLen;
  unsigned char reserved;
  unsigned char orders[128];
};

int goAhead(const char* path) {
  int chans=4;
  struct MODFile mod;

  f=fopen(path,"rb");
  if (f==NULL) {
    perror(path);
    return 1;
  }

  // quick and dirty MOD reader
  char mk[4];

  if (fseek(f,1080,SEEK_SET)!=0) {
    fprintf(stderr,"%s: could not seek to magic: %s\n",path,strerror(errno));
    fclose(f);
    return 1;
  }

  if (fread(mk,1,4,f)!=4) {
    if (ferror(f)) {
      fprintf(stderr,"%s: could not read magic: %s\n",path,strerror(errno));
      fclose(f);
      return 1;
    }
    if (feof(f)) {
      fprintf(stderr,"%s: could not read magic: end of file\n",path);
      fclose(f);
      return 1;
    }
    fprintf(stderr,"%s: could not read magic: unknown error?\n",path);
    fclose(f);
    return 1;
  }

  if (memcmp(mk,"M.K.",4)==0 ||
      memcmp(mk,"M!K!",4)==0 ||
      memcmp(mk,"M&K!",4)==0) {
    chans=4;
  } else if (memcmp(&mk[1],"CHN",3)==0 && mk[0]>='0' && mk[0]<='9') {
    chans=mk[0]-'0';
  } else if (memcmp(mk,"FLT",3)==0 && mk[3]>='0' && mk[3]<='9') {
    chans=mk[3]-'0';
  } else if (memcmp(mk,"TDZ",3)==0 && mk[3]>='0' && mk[3]<='9') {
    chans=mk[3]-'0';
  } else if ((memcmp(&mk[2],"CH",2)==0 || memcmp(&mk[2],"CN",2)==0) && mk[0]>='0' && mk[0]<='9' && mk[1]>='0' && mk[1]<='9') {
    chans=((mk[0]-'0')*10)+(mk[1]-'0');
  } else if (memcmp(mk,"CD81",4)==0 || memcmp(mk,"OKTA",4)==0 || memcmp(mk,"OCTA",4)==0) {
    chans=8;
  } else {
    fprintf(stderr,"%s: not a valid .mod\n",path);
    fclose(f);
    return 1;
  }

  if (chans<1) {
    fprintf(stderr,"%s: invalid channel count!\n",path);
    fclose(f);
    return 1;
  }

  if (fseek(f,0,SEEK_SET)!=0) {
    fprintf(stderr,"%s: could not seek to beginning of file: %s\n",path,strerror(errno));
    fclose(f);
    return 1;
  }

  // read .mod
  if (fread(&mod,1,1080,f)!=1080) {
    if (ferror(f)) {
      fprintf(stderr,"%s: could not read module data: %s\n",path,strerror(errno));
      fclose(f);
      return 1;
    }
    if (feof(f)) {
      fprintf(stderr,"%s: could not read module data: end of file\n",path);
      fclose(f);
      return 1;
    }
    fprintf(stderr,"%s: could not read module data: unknown error?\n",path);
    fclose(f);
    return 1;
  }

  // seek by pattern count
  int maxPat=0;
  for (int i=0; i<128; i++) {
    if (mod.orders[i]>maxPat) maxPat=mod.orders[i];
  }
  maxPat++;

  if (fseek(f,4+64*4*chans*maxPat,SEEK_CUR)!=0) {
    fprintf(stderr,"%s: could not seek to beginning of sample data: %s\n",path,strerror(errno));
    fclose(f);
    return 1;
  }

  memcpy(str,mod.modName,20);
  str[20]=0;
  printf("> %s: %s\n",path,str);

  for (int i=0; i<31; i++) {
    memcpy(str,mod.ins[i].name,22);
    str[22]=0;
    printf("  - %s\n",str);

    const char* fileBase=mod.modName;
    if (fileBase[0]==0) {
      fileBase=strrchr(path,DIR_SEPARATOR);
      if (fileBase==NULL) {
        fileBase=path;
      }
    }

    int strPos=0;
    for (int j=0; j<20; j++) {
      if (!fileBase[j]) break;
      if (fileBase[j]<' ' ||
          fileBase[j]=='<' ||
          fileBase[j]=='>' ||
          fileBase[j]==':' ||
          fileBase[j]=='"' ||
          fileBase[j]=='/' ||
          fileBase[j]=='\\' ||
          fileBase[j]=='|' ||
          fileBase[j]=='?' ||
          fileBase[j]=='*' ||
          fileBase[j]>0x7e) {
        str[strPos++]='_';
      } else {
        str[strPos++]=fileBase[j];
      }
    }

    str[strPos++]='-';
    strPos+=snprintf(&str[strPos],1024,"%d-",i+1);

    fileBase=mod.ins[i].name;    
    for (int j=0; j<22; j++) {
      if (!fileBase[j]) break;
      if (fileBase[j]<' ' ||
          fileBase[j]=='<' ||
          fileBase[j]=='>' ||
          fileBase[j]==':' ||
          fileBase[j]=='"' ||
          fileBase[j]=='/' ||
          fileBase[j]=='\\' ||
          fileBase[j]=='|' ||
          fileBase[j]=='?' ||
          fileBase[j]=='*' ||
          fileBase[j]>0x7e) {
        str[strPos++]='_';
      } else {
        str[strPos++]=fileBase[j];
      }
    }
    str[strPos]=0;
    strncat(str,".wav",4095);

    unsigned int actualLen=(((mod.ins[i].len>>8)|(mod.ins[i].len<<8))&0xffff)<<1;
    if (actualLen==0) continue;

    unsigned char* data=malloc(actualLen);
    if (data==NULL) {
      fprintf(stderr,"out of memory!\n");
      continue;
    }
    
    size_t howMuch=fread(data,1,actualLen,f);
    if (howMuch!=actualLen) {
      if (ferror(f)) {
        fprintf(stderr,"%s: could not read sample data: %s\n",path,strerror(errno));
      }
      if (feof(f)) {
        fprintf(stderr,"%s: could not read sample data: end of file\n",path);
      }
    }
    if (howMuch==0) {
      free(data);
      continue;
    }

    for (size_t j=0; j<howMuch; j++) {
      data[j]^=0x80;
    }

    SF_INFO si;
    memset(&si,0,sizeof(SF_INFO));
    si.samplerate=sampleRates[mod.ins[i].fine&7];
    si.channels=1;
    si.format=SF_FORMAT_WAV|SF_FORMAT_PCM_U8;

    SNDFILE* sf=sf_open(str,SFM_WRITE,&si);
    if (sf==NULL) {
      fprintf(stderr,"%s: could not open .wav file: %s\n",path,sf_strerror(NULL));
      free(data);
      continue;
    }

    if (sf_write_raw(sf,data,howMuch)!=(sf_count_t)howMuch) {
      fprintf(stderr,"%s: did not write entirely\n",path);
    }

    sf_close(sf);

    free(data);
  }

  fclose(f);
  return 0;
}

int main(int argc, char** argv) {
  if (argc<2) {
    printf("usage: %s file ...\nextract samples from .mod files.\n",argv[0]);
    return 1;
  }

  int hasErr=0;
  for (int i=1; i<argc; i++) {
    if (goAhead(argv[i])!=0) {
      hasErr=1;
    }
  }
  if (hasErr!=0) {
    fprintf(stderr,"exiting with failure due to previous errors.\n");
  }
  return hasErr;
}
