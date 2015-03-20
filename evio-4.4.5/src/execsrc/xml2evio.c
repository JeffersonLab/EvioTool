/*
 *  xml2evio.c
 *
 *   Converts (almost) arbitrary xml data to binary evio using data dictionary
 *     Full support for tagsegments and 64-bit
 *     Does NOT handle VAX float or double, packets, or repeating structures
 *
 *
 *   XML rules:
 *      containers (tags) MUST contain EITHER data OR other containers
 *      data in a container MUST be all the same type
 *      
 *
 *   NOTES:
 *     Assumes valid xml input so does minimal checking
 *       ...parsing inconsistant input will result in bizarre behavior
 *     Current parser (expat) is non-validating
 *
 *
 *   Input file rules:
 *     default is stdin, no gzip input allowed
 *     if filename specified:
 *        if file is in gzip format opens pipe to gunzip
 *        otherwise just opens file (n.b. "-" means use stdin, no gzip)
 *
 *
 *   Author: Elliott Wolin, JLab, 3-jul-2001
*/


/* still to do
 * -----------
 * store attributes
 *
*/



/* for posix */
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__


/* macros */
#define MAXXMLBUF  500000
#define MAXCDATA   500000
#define MAXEVIOBUF 500000
#define MAXDEPTH   100
#define MAXDICT    5000
#define min(a, b)  ((a) > (b) ? (b) : (a))


/* include files */
#include <evio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>

#ifndef _MSC_VER
#include <zlib.h>
#endif


/* file variables */
static char *xmlfilename  = (char*)"-";
static char *eviofilename = (char*)"xml2evio.dat";
static char *dictfilename = NULL;
static int eviohandle;


/* input and output buffers */
static char xmlbuf[MAXXMLBUF];
static char cdata[MAXCDATA];
static char *pcdata=cdata;
static int cdatalen       = 0;
static uint32_t eviobuf[MAXEVIOBUF];
static int eviolen        = 0;


/* xml tag dictionary */
typedef struct {
  char *name;
  int data_type;
  int tag;
  int num;
} dict_entry;
static dict_entry dict[MAXDICT];
static int ndict          = 0;


/* stack variables */
typedef struct {
  int addr;
  int content;
  int len;
  int noexpand;
} stack_entry;
static stack_entry stack[MAXDEPTH];
static int nstack         = 0;


/* for skipping unknown tags */
static int event_started  = 0;
static char unknown_tag[128];
static int unknown_count  = 0;


/*  misc variables */
static char *main_tag     = (char*)"evio-data";
static char *event_tag    = (char*)"event";
static XML_Parser xmlParser;
static XML_Parser dictParser;
static int nevent         = 0;
static int nskip          = 0;
static int max_event      = 0;
static int skipit         = 0;
static int done           = 0;
static int debug          = 0;


/* prototypes */
void decode_command_line(int argc, char **argv);
void create_dictionary(void);
void set_dict_defaults(void);
FILE *open_xml_file(char *xmlfilename);
void startDictElement(void *userData, const char *name, const char **atts);
void startDataElement(void *userData, const char *name, const char **atts);
void endDataElement(void *userData, const char *name);
void get_params(const char *name, const char **atts, int natt, 
	       int *tag, int *data_type, int *num, int *ptype, int *noexpand);
int ignore_start_tag(const char *name);
int ignore_end_tag(const char *name);
void dataHandler(void *userData,const XML_Char *s, int len);
void decode_cdata(int type, int *nword, int noexpand);
void push_stack(stack_entry *a);
stack_entry *pop_stack(void);
stack_entry *get_stack(void);
int find_tag(const char **atts, const int natt, const char *tag);
int get_int_tag(const char **atts, const int natt, const char *tag, int *val);
const char *get_char_tag(const char **atts, const int natt, const char *tag);

/*--------------------------------------------------------------------------*/


int main (int argc, char **argv) {

  int status,l;
  size_t len;
  FILE *xmlfile;
  

  /* decode command line */
  decode_command_line(argc,argv);


  /* create tag dictionary */
  create_dictionary();


  /* open xml input file */
  if((xmlfile=open_xml_file(xmlfilename))==NULL) {
    printf("\n ?Unable to open xml data file %s\n\n",xmlfilename);
    exit(EXIT_FAILURE);
  }


  /* open evio (binary) output file */
  if((status=evOpen(eviofilename,"w",&eviohandle))!=0) {
    printf("\n ?Unable to open evio output file %s, status=%d\n\n",eviofilename,status);
    exit(EXIT_FAILURE);
  }

  /* ??? use large block size */
  l=0x8000;
  evIoctl(eviohandle,"b",(void*)&l);


  /* create parser and set callbacks */
  xmlParser=XML_ParserCreate(NULL);
  XML_SetElementHandler(xmlParser,startDataElement,endDataElement);
  XML_SetCharacterDataHandler(xmlParser,dataHandler);


  /* read event fragments from file and pass to parser */
  while (!done) {
#ifdef _MSC_VER
    len=(int)read(xmlfile,xmlbuf,MAXXMLBUF);
#else
    len=(int)gzread(xmlfile,xmlbuf,MAXXMLBUF);
#endif
    XML_Parse(xmlParser,xmlbuf,len,len<MAXXMLBUF/4);
    done=done||(len<MAXXMLBUF/4);
  }


  /* done */
  evClose(eviohandle);
  exit(EXIT_SUCCESS);
}


/*---------------------------------------------------------------- */


void create_dictionary(void) {

  FILE *dictfile;
  int len;


  /* set defaults */
  set_dict_defaults();


  /* is dictionary file specified */
  if(dictfilename==NULL)return;


  /* read add'l tags from xml dictionary file */
  if((dictfile=fopen(dictfilename,"r"))==NULL) {
    printf("\n?unable to open dictionary file %s\n\n",dictfilename);
    exit(EXIT_FAILURE);
  }


  /* create parser and set callbacks */
  dictParser=XML_ParserCreate(NULL);
  XML_SetElementHandler(dictParser,startDictElement,NULL);


  /* read and parse dictionary file */
  do {
    len=fread(xmlbuf,1,MAXXMLBUF,dictfile);
    XML_Parse(dictParser,xmlbuf,len,len<MAXXMLBUF);
  } while (len==MAXXMLBUF);
  

  fclose(dictfile);
  return;
}


/*---------------------------------------------------------------- */


void startDictElement(void *userData, const char *name, const char **atts) {

  int natt=XML_GetSpecifiedAttributeCount(dictParser);
  int data_type=0,num=0;
  const char *tag, *p;


  if(strcasecmp(name,"evioDictEntry")!=0)return;

  ndict++;
  if(ndict>MAXDICT) {
    printf("\n?dictionary too large\n\n");
    exit(EXIT_FAILURE);
  }
  
  /* store dictionary info */
  dict[ndict-1].name=strdup(get_char_tag(atts,natt,"name"));

  get_int_tag(atts,natt,"data_type",&data_type);
  dict[ndict-1].data_type=data_type;

  tag=get_char_tag(atts,natt,"tag");
  if((p=strrchr(tag,'.'))==NULL)p=tag-1;
  dict[ndict-1].tag=atoi(p+1);

  get_int_tag(atts,natt,"num",&num);
  dict[ndict-1].num=num;

  return;
}


/*---------------------------------------------------------------- */


FILE *open_xml_file(char *xmlfilename) {

  int i=strspn(xmlfilename," \n\t");


  if(xmlfilename[i]=='-') {
    return(stdin);  /* no gzip input allowed from stdin */

  } else if(xmlfilename[i]=='|') {
    return(popen(xmlfilename+i+1,"r"));

  } else {
#ifdef _MSC_VER
    return(open(xmlfilename+i,"r"));
#else
    return(gzopen(xmlfilename+i,"r"));
#endif
  }

}


/*---------------------------------------------------------------- */


void startDataElement(void *userData, const char *name, const char **atts) {

  stack_entry s;
  const char *fmt;
  int tag,data_type,num,ptype,noexp;
  int natt=XML_GetSpecifiedAttributeCount(xmlParser);    


  /* wait for start of event, skip event if other than evio format specified */
  /* ignore main_tag */
  if((!event_started)&&(strcasecmp(name,main_tag)==0)) {
    return;
  } else if(strcasecmp(name,event_tag)==0) {
    event_started=1;
    nevent++;
    fmt=get_char_tag(atts,natt,"format");
    skipit = done || (nskip>=nevent) || ((fmt!=NULL)&&(strcasecmp(fmt,"evio")!=0));
  } else if (!event_started) {
    if(debug!=0)printf("  ?start tag %s is outside event, nevent = %d\n",name,nevent);
  }


  /* inside an event */
  if(!skipit && !ignore_start_tag(name)) {

    get_params(name,atts,natt,&tag,&data_type,&num,&ptype,&noexp);
    s.addr=eviolen;
    s.content=data_type;
    s.noexpand=noexp;
    
    
    switch (ptype) { 
      
    case(0xe):
    case(0x10):
      s.len=2;
      eviobuf[eviolen++]=0;
      eviobuf[eviolen++]=(tag<<16) | (data_type<<8) | num;
      break;
      
    case(0xd):
    case(0x20):
      s.len=1;
      eviobuf[eviolen++]=(tag<<24) | (data_type<<16);
      break;
      
    case(0xc):
    case(0x40):
      s.len=1;
      eviobuf[eviolen++]=(tag<<20) | (data_type<<16);
      break;
    }

    push_stack(&s);
    cdata[0]='\0';
    pcdata=cdata;
    cdatalen=0;
  }
  
  return;
}


/*---------------------------------------------------------------- */


void endDataElement(void *userData, const char *name) {

  stack_entry *s, *ps;
  int nword;


  /* ignore tag if event not started, error if not main_tag */
  if(!event_started) {
    if((debug!=0)&&strcasecmp(name,main_tag)!=0) {
      printf("  ?end tag %s is outside event, nevent = %d\n",name,nevent);
    }
    return;
  }


  /* inside an event */
  if(!skipit && !ignore_end_tag(name)) {
    
    s=pop_stack();
    ps=get_stack();
    
    if((s->noexpand==0) &&
       ((s->content== 0xc)||(s->content== 0xd)||(s->content== 0xe)||
       (s->content==0x10)||(s->content==0x20)||(s->content==0x40))) {
      eviobuf[s->addr]+=s->len-1;
      if(ps!=NULL)ps->len+=s->len;

    } else {

      decode_cdata(s->content,&nword,s->noexpand);
      eviobuf[s->addr]+=s->len+nword-1;
      if(ps!=NULL)ps->len+=s->len+nword;
    }
    cdata[0]='\0';
    pcdata=cdata;
    cdatalen=0;
  }
  

  /* end of event */
  if(strcasecmp(name,event_tag)==0) {
    event_started=0;

    if(nstack!=0) {
      printf("\n?stack error at end of event %d, nstack = %d\n\n",nevent,nstack);
      exit(EXIT_FAILURE);
    }
    if(!skipit) {
      evWrite(eviohandle,eviobuf);
      eviolen=0;
    }      
    done=(max_event>0)&&(nevent>=max_event+nskip);
/*      return; */
  }
    

  return;
}


/*---------------------------------------------------------------- */


void get_params(const char *name, const char **atts, int natt, 
		int *tag, int *data_type, int *num, int *ptype, int *noexpand) {

  stack_entry *ps;
  const char *opt;
  int i;


  // check for new substring element
  if(strcasecmp(name,"str")==0) {
    *tag=0;
    *num=0;
    *data_type=0x3;
    *ptype=0x3;  // ???
    *noexpand=0;
    return;
  }
  


  /* check dictionary for defaults */
  for(i=0; i<ndict; i++) {
    if(strcasecmp(name,dict[i].name)==0) {
      *data_type=dict[i].data_type;
      *tag=dict[i].tag;
      *num=dict[i].num;
    }
  }


  /* explicit xml attributes override defaults */
  get_int_tag(atts,natt,"data_type",data_type);
  get_int_tag(atts,natt,"tag",tag);
  get_int_tag(atts,natt,"num",num);


  /* check if data not expanded */
  opt=get_char_tag(atts,natt,"opt");
  *noexpand=((opt!=NULL)&&(strcasecmp(opt,"noexpand")==0))?1:0;


  /* get content type */
  if((ps=get_stack())!=NULL) {
    *ptype=ps->content;
  } else {
    *ptype=0x10;   /* outermost fragment is always a bank */
  }
  

  /* consistency check */
  if(debug!=0) {
    if( 
       ((strcasecmp(name,"bank")      ==0)&&((*ptype!=0xe)&&(*ptype!=0x10))) ||
       ((strcasecmp(name,"segment")   ==0)&&((*ptype!=0xd)&&(*ptype!=0x20))) ||
       ((strcasecmp(name,"tagsegment")==0)&&((*ptype!=0xc)&&(*ptype!=0x40))) 
       ) 
      printf("?warning...%s has parent %x in event %d\n",name,*ptype,nevent);
  }


  return;
}


/*---------------------------------------------------------------- */


int ignore_start_tag(const char *name) {

  int i;


  /* side effect is to turn on unknown_tag mode */
  if(unknown_count>0) {
    if(strcasecmp(name,unknown_tag)==0)unknown_count++;
    return(1);
  }


  /* search for tag in list */
  for(i=0; i<ndict; i++) if(strcasecmp(name,dict[i].name)==0)return(0);


  /* new unknown tag...set flag and save name */
  if(debug!=0)printf("  ?ignoring unknown tag %s in event %d\n",name,nevent);
  unknown_count=1;
  strcpy(unknown_tag,name);
  return(1);
}


/*---------------------------------------------------------------- */


int ignore_end_tag(const char *name) {

  int i;


  /* side effect is to turn off unknown_tag mode */
  if(unknown_count>0) {
    if(strcasecmp(name,unknown_tag)==0)unknown_count--;
    return(1);
  }


  /* find tag in list */
  for(i=0; i<ndict; i++) if(strcasecmp(name,dict[i].name)==0)return(0);


  /* shouldn't get here */
  printf("\n?non-ignored end tag NOT in list, event %d, name = %s\n\n",nevent,name);
  exit(EXIT_FAILURE);
}


/*---------------------------------------------------------------- */


void dataHandler(void *userData, const XML_Char *s, int len) {


  if(skipit)return;
/*    if(len<1)return; */
  cdatalen+=len;
  if(cdatalen>=MAXCDATA) {
    printf("\n?cdata too large\n\n");
    exit(EXIT_FAILURE);
  }
  pcdata=strncat(pcdata,s,len);
  return;
}


/*---------------------------------------------------------------- */


void decode_cdata(int content, int *nword, int noexpand) {
  
  int i,n=0,len,nw=0;
  char *p, *c;

  int l;
  unsigned int ul;
  short s;
  unsigned short us;
  float f;
  double d;
  long long ll;
  unsigned long long ull;


  /* just copy words if data not expanded */
  if(noexpand) {
    p=strtok(cdata," \n\t");
    while(p!=NULL) {
      n++;
      if(strncasecmp(p,"0x",2)==0) {
	  sscanf(p+2,"%x%n",&ul,&len);
	  len+=2;
      } else {
	  sscanf(p,"%d%n",&ul,&len);
      }
      ((unsigned int*)(&eviobuf[eviolen]))[n-1]=ul;
      p=strtok(NULL," \n\t");
    }
    *nword=n;
    eviolen+=n;
    return;
  }


  /* data encoded */
  switch (content) {

  case 0x1:
    p=strtok(cdata," \n\t");
    while(p!=NULL) {
      n++;
      if(strncasecmp(p,"0x",2)==0) {
	  sscanf(p+2,"%x%n",&ul,&len);
	  len+=2;
      } else {
	  sscanf(p,"%d%n",&ul,&len);
      }
      ((unsigned int*)(&eviobuf[eviolen]))[n-1]=ul;
      p=strtok(NULL," \n\t");
    }
    nw=n;
    break;

  case 0x2:
    p=strtok(cdata," \n\t");
    while(p!=NULL) {
      n++;
      sscanf(p,"%f",&f);
      ((float*)(&eviobuf[eviolen]))[n-1]=f;
      p=strtok(NULL," \n\t");
    }
    nw=n;
    break;

  case 0x3:
    p=cdata+strspn(cdata,"\n");
/* ??? */
    i=strlen(p);    
    while((i>0)&&((p[i-1]==' ')||(p[i-1]=='\n')))p[(i--)-1]='\0';
    strcpy((char*)(&eviobuf[eviolen]),p);
    nw=(i+3)/4;
    break;

  case 0x4:
    p=strtok(cdata," \n\t");
    while(p!=NULL) {
      n++;
      sscanf(p,"%hd",&s);
      ((short*)(&eviobuf[eviolen]))[n-1]=s;
      p=strtok(NULL," \n\t");
    }
    nw=(n+1)/2;    
    break;

  case 0x5:
    p=strtok(cdata," \n\t");
    while(p!=NULL) {
      n++;
      if(strncasecmp(p,"0x",2)==0) {
	  sscanf(p+2,"%hx%n",&us,&len);
	  len+=2;
      } else {
	  sscanf(p,"%hx%n",&us,&len);
      }
      ((unsigned short*)(&eviobuf[eviolen]))[n-1]=us;
      p=strtok(NULL," \n\t");
    }
    nw=(n+1)/2;    
    break;

  case 0x6:
    p=strtok(cdata," \n\t");
    c=(char*)(&eviobuf[eviolen]);
    while(p!=NULL) {
      n++;
      sscanf(p,"%i",&i);
      c[n-1]=i&0xff;
      p=strtok(NULL," \n\t");
    }
    nw=(n+3)/4;
    break;

  case 0x7:
    p=strtok(cdata," \n\t");
    c=(char*)(&eviobuf[eviolen]);
    while(p!=NULL) {
      n++;
      if(strncasecmp(p,"0x",2)==0) {
	  sscanf(p+2,"%hx%n",&us,&len);
	  len+=2;
      } else {
	  sscanf(p,"%hx%n",&us,&len);
      }
      c[n-1]=(char)us&0xff;
      p=strtok(NULL," \n\t");
    }
    nw=(n+3)/4;
    break;

  case 0x8:
    p=strtok(cdata," \n\t");
    while(p!=NULL) {
      n++;
      sscanf(p,"%le",&d);
      ((double*)(&eviobuf[eviolen]))[n-1]=d;
      p=strtok(NULL," \n\t");
    }
    nw=2*n;
    break;

  case 0x9:
    p=strtok(cdata," \n\t");
    while(p!=NULL) {
      n++;
      sscanf(p,"%lld",&ll);
      ((long long*)(&eviobuf[eviolen]))[n-1]=ll;
      p=strtok(NULL," \n\t");
    }
    nw=2*n;
    break;

  case 0xa:
    p=strtok(cdata," \n\t");
    while(p!=NULL) {
      n++;
      if(strncasecmp(p,"0x",2)==0) {
	  sscanf(p+2,"%llx%n",&ull,&len);
	  len+=2;
      } else {
	  sscanf(p,"%llx%n",&ull,&len);
      }
      ((unsigned long long*)(&eviobuf[eviolen]))[n-1]=ull;
      p=strtok(NULL," \n\t");
    }
    nw=2*n;
    break;

  case 0xb:
    p=strtok(cdata," \n\t");
    while(p!=NULL) {
      n++;
      sscanf(p,"%d",&l);
      ((int*)(&eviobuf[eviolen]))[n-1]=l;
      p=strtok(NULL," \n\t");
    }
    nw=n;
    break;

  default:
    nw=0;
    break;
  }

  *nword=nw;
  eviolen+=nw;
  return;
}


/*---------------------------------------------------------------- */


void push_stack(stack_entry *s) {

  nstack++;
  if(nstack>MAXDEPTH) {
    printf("?stack overflow\n");
    exit(EXIT_FAILURE);
  }

  stack[nstack-1]=*s;

  return;
}


/*---------------------------------------------------------------- */


stack_entry *pop_stack(void) {

  stack_entry *s=get_stack();

  if(s==NULL) {
    printf("?stack underflow\n");
    exit(EXIT_FAILURE);
  } else {
    nstack--;
    return(s);
  }
}


/*---------------------------------------------------------------- */


stack_entry *get_stack() {

  if(nstack<1) {
    return(NULL);
  } else {
    return(&stack[nstack-1]);
  }
}


/*---------------------------------------------------------------- */


int find_tag(const char **atts, const int natt, const char *name) {

  /* return 1 if att exists, 0 otherwise */

  int i;

  for(i=0; i<natt; i+=2)if(strcasecmp(atts[i],name)==0)return(1);
  return(0);
}


/*---------------------------------------------------------------- */


int get_int_tag(const char **atts, const int natt, const char *name, int *val) {

  int i;

  for(i=0; i<natt; i+=2) {
    if(strcasecmp(atts[i],name)==0) {
      if(strncmp(atts[i+1],"0x",2)==0) {
	sscanf(atts[i+1],"%x",val);
      } else if(strncmp(atts[i+1],"0X",2)==0) {
	sscanf(atts[i+1],"%X",val);
      } else {
	sscanf(atts[i+1],"%d",val);
      }
      return(1);
    }
  }

  return(0);
}


/*---------------------------------------------------------------- */


const char *get_char_tag(const char **atts, const int natt, const char *name) {

  int i;

  for(i=0; i<natt; i+=2) {
    if(strcasecmp(atts[i],name)==0)return((const char*)atts[i+1]);
  }

  return(NULL);
}


/*---------------------------------------------------------------- */


void decode_command_line(int argc, char**argv) {
  
  const char *help = 
    "\nusage:\n\n  xml2evio [-xml xmlfilename] [-max max_event] [-skip nskip] [-evio eviofilename] [-dict dictfilename] [-m main_tag] [-e event_tag]\n";
  int i;
    
    
  /* loop over arguments */
  i=1;
  while (i<argc) {
    if (strncasecmp(argv[i],"-h",2)==0) {
      printf("%s\n",help);
      exit(EXIT_SUCCESS);

    } else if (strncasecmp(argv[i],"-debug",6)==0) {
      debug=1;
      i=i+1;

    } else if (strncasecmp(argv[i],"-max",4)==0) {
      max_event=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-skip",5)==0) {
      nskip=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-xml",4)==0) {
      xmlfilename=strdup(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-evio",5)==0) {
      eviofilename=strdup(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-dict",5)==0) {
      dictfilename=strdup(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-m",2)==0) {
      main_tag=strdup(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-e",2)==0) {
      event_tag=strdup(argv[i+1]);
      i=i+2;

    } else {
      break;
    }
  }
  

  return;
}


/*---------------------------------------------------------------- */


void set_dict_defaults(void) {


  /* define standard tags, default contents are tagsegments */
  ndict++;
  dict[ndict-1].name=strdup(event_tag);
  dict[ndict-1].data_type=0x20;

  ndict++;
  dict[ndict-1].name=strdup("unknown32");
  dict[ndict-1].data_type=0x0;

  ndict++;
  dict[ndict-1].name=strdup("uint32");
  dict[ndict-1].data_type=0x1;

  ndict++;
  dict[ndict-1].name=strdup("float32");
  dict[ndict-1].data_type=0x2;

  ndict++;
  dict[ndict-1].name=strdup("string");
  dict[ndict-1].data_type=0x3;

  ndict++;
  dict[ndict-1].name=strdup("int16");
  dict[ndict-1].data_type=0x4;

  ndict++;
  dict[ndict-1].name=strdup("uint16");
  dict[ndict-1].data_type=0x5;

  ndict++;
  dict[ndict-1].name=strdup("int8");
  dict[ndict-1].data_type=0x6;

  ndict++;
  dict[ndict-1].name=strdup("uint8");
  dict[ndict-1].data_type=0x7;

  ndict++;
  dict[ndict-1].name=strdup("float64");
  dict[ndict-1].data_type=0x8;

  ndict++;
  dict[ndict-1].name=strdup("int64");
  dict[ndict-1].data_type=0x9;

  ndict++;
  dict[ndict-1].name=strdup("uint64");
  dict[ndict-1].data_type=0xa;

  ndict++;
  dict[ndict-1].name=strdup("int32");
  dict[ndict-1].data_type=0xb;


  /* containers default to contain banks */
  ndict++;
  dict[ndict-1].name=strdup("bank");
  dict[ndict-1].data_type=0xe;

  ndict++;
  dict[ndict-1].name=strdup("segment");
  dict[ndict-1].data_type=0xe;

  ndict++;
  dict[ndict-1].name=strdup("tagsegment");
  dict[ndict-1].data_type=0xe;

  return;
}


/*---------------------------------------------------------------- */
