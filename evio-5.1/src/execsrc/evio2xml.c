/*
 *  evio2xml.c
 *
 *   Converts binary evio data to xml, full support for tagsegments and 64-bit
 *
 *   NOTE:  does NOT handle VAX float or double, packets, or repeating structures
 *
 *   Use -gz to write output file using gzip, but not on windows
 *
 *   Author: Elliott Wolin, JLab, 6-sep-2001
 *
 *   Carl Timmer, Jan 2012, evio version 4
 *   Merged src/libsrc/xml_util.c with this file.
 *   This takes all xml formatting stuff out of evio library.
 *
 *   Sergei Boiarinov added composite data handling
 */



/* container types used locally */
enum {
    BANK = 0,
    SEGMENT,
    TAGSEGMENT,
};

/* for posix */
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__


/*  misc macros, etc. */
#define MAXXMLBUF  100000
#define MAXDICT    5000
#define MAXDEPTH   512
#define min(a, b)  ( ( (a) > (b) ) ? (b) : (a) )


#define MAXEVIOBUF   10000000
#define EVIO2XML     5


/* include files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <expat.h>
#include "evio.h"


/*  misc variables from orig evio2xml.c */
static char *filename;
static char *dictfilename = NULL;
static char *dicttagname  = "dictEntry";
static char *outfilename  = NULL;
static int gzip           = 0;
static char *main_tag     = (char*)"evio-data";
static int nevent         = 0;
static int skip_event     = 0;
static int max_event      = 0;
static int nevok          = 0;
static int evok[100];
static int nnoev          = 0;
static int noev[100];
static int nfragok        = 0;
static int fragok[100];
static int nnofrag        = 0;
static int nofrag[100];
static int pause          = 0;
static int maxbuf         = MAXEVIOBUF;
static int debug          = 0;
static int done           = 0;


/*  misc variables from orig xml_util.c */


/* xml tag dictionary */
static XML_Parser dictParser;
static char *xmlbuf;
typedef struct {
    char *name;
    int ntag;
    int *tag;
    int nnum;
    int *num;
} dict_entry;
static dict_entry dict[MAXDICT];
static int ndict = 0;


/* fragment info */
char *fragment_name[] = {"bank","segment","tagsegment"};
int fragment_offset[] = {2,1,1};


/* formatting info */
static int xtod         = 0;
static int n8           = 8;
static int n16          = 8;
static int n32          = 4;
static int n64          = 2;


/*  misc variables */
static int nbuf;
static char *event_tag    = "event";
static char *bank2_tag    = "bank";
static char *dicttagname2 = NULL;
static int max_depth      = -1;
static int depth          = 0;
static int tagstack[MAXDEPTH];
static int numstack[MAXDEPTH];
static int no_typename    = 0;
static int verbose        = 0;
static int brief          = 0;
static int no_data        = 0;


/*********** XML INDENTATION ********************/
static char *xml;

static char indentStr[3*MAXDEPTH + 1];

static void indent() {
    xml += sprintf(xml, indentStr);
}

static int getIndent() {
    return (int)strlen(indentStr);
}

/** Routine to increase the xml indentation by 3 spaces up to the limit. */
static void increaseIndent() {
    int i, len = strlen(indentStr);
    if (len > 3*MAXDEPTH) return;

    for (i=len-1; i<len+3; i++) {
        indentStr[i] = ' ';
    }
    indentStr[len+3] = '\0';
}

/** Routine to decrease the xml indentation by 3 spaces down to 0. */
static void decreaseIndent() {
    int i, len = strlen(indentStr);
    if (len < 3) return;
    indentStr[len-3] = '\0';
}

/** Routine to increase the indentation by 3 spaces up to the limit
 *  and then add the indentation to the xml string being created. */
static void increaseAndIndent() {
    increaseIndent();
    indent();
}

/** Routine to decrease the indentation by 3 spaces down to 0
 *  and then add the indentation to the xml string being created. */
static void decreaseAndIndent() {
    decreaseIndent();
    indent();
}
/************************************************/


/* xml_util.c prototypes */
static void create_dictionary(char *dictfilename);
static void startDictElement(void *userData, const char *name, const char **atts);
static void dump_fragment(unsigned int *buf, int fragment_type);
static void dump_data(unsigned int *data, int type, int length, int padding, int noexpand);
static int  get_ndata(int type, int nwords, int padding);
static const char *get_matchname();
static const char *get_char_att(const char **atts, const int natt, const char *tag);

/* user supplied fragment select function via set_user_frag_select_func(int (*f) (int tag)) */
static int (*USER_FRAG_SELECT_FUNC) (int tag) = NULL;

/* From C lib */
extern int eviofmtdump(int *iarr, int nwrd, unsigned char *ifmt, int nfmt,
                int nextrabytes, int numIndent, int hex, char *xmlStringx);
extern int eviofmt(char *fmt, unsigned char *ifmt, int ifmtLen);

/* evio2xml.c prototypes */
void decode_command_line(int argc, char **argv);
void evio_xmldump_init(char *dictfilename, char *dicttagname);
void evio_xmldump(unsigned int *buf, int evnum, char *string, int len);
void evio_xmldump_done(char *string, int len);
void writeit(FILE *f, char *s, int len);
int user_event_select(unsigned int *buf);
int user_frag_select(int tag);

#ifndef _MSC_VER
FILE *gzopen(char*,char*);
void gzclose(FILE*);
void gzwrite(FILE*,char*,int);
#endif

int set_event_tag(char *tag);
int set_bank2_tag(char *tag);
int set_n8(int val);
int set_n16(int val);
int set_n32(int val);
int set_n64(int val);
int set_xtod(int val);
int set_max_depth(int val);
int set_no_typename(int val);
int set_verbose(int val);
int set_brief(int val);
int set_no_data(int val);


/*--------------------------------------------------------------------------*/


int main (int argc, char **argv)
{

    int handle, status;
    FILE *out = NULL;
    char s[256];


    /* decode command line */
    decode_command_line(argc,argv);

    /*allocate xml buffer*/
    xmlbuf = malloc(MAXXMLBUF);

    /* allocate binary buffer and string buffer (EVIO2XML times as large) */
    unsigned int *buf = (unsigned int*)malloc(maxbuf*sizeof(unsigned int));
    char *xml = malloc(maxbuf*sizeof(unsigned int)*EVIO2XML);

    if((buf==NULL)||(xml==NULL)) {
        int sz=maxbuf*sizeof(unsigned int);
        printf("\n   *** Unable to allocate buffers ***\n\n");
        printf("\n buf size=%d bytes, addr=0x%x     xml size=%d bytes, addr=0x%x\n\n",sz,buf,
               sz*EVIO2XML,xml);
        exit(EXIT_FAILURE);
    }


    /* Initialize indent string */
    indentStr[0] = '\0';


    /* open evio input file */
    if((status=evOpen(filename,"r",&handle))!=0) {
        printf("\n ?Unable to open file %s, status=%d, 0x%x\n\n",filename,status, status);
        exit(EXIT_FAILURE);
    }


    /* open output file, gzip format if requested */
    if(outfilename!=NULL) {
        if(gzip==0) {
            out=fopen(outfilename,"w");
#ifndef _MSC_VER
        } else {
            out=(FILE*)gzopen(outfilename,"wb");
#endif
        }
    }


    /* init xmldump */
    set_user_frag_select_func(user_frag_select);
    evio_xmldump_init(dictfilename,dicttagname);
    sprintf(s,"<?xml version=\"1.0\"?>\n");
    writeit(out,s,strlen(s));
    sprintf(s,"<%s>\n",main_tag);
    writeit(out,s,strlen(s));

    increaseIndent();

    /* loop over events, perhaps skip some, dump up to max_event events */
    nevent=0;
    while ((status=evRead(handle,buf,maxbuf))==0) {
        nevent++;
        if(skip_event>=nevent){printf("cont1\n");continue;}
        if(user_event_select(buf)==0){printf("cont2\n");continue;}
        xml[0]='\0';              /* clear xml string buffer */
        evio_xmldump(buf,nevent,xml,maxbuf*sizeof(unsigned int)*EVIO2XML);
        writeit(out,xml,strlen(xml));


        if(pause!=0) {
            printf("\n\nHit return to continue, q to quit: ");
            fgets(s,sizeof(s),stdin);
            if(tolower(s[strspn(s," \t")])=='q')done=1;
        }

        if((done!=0)||((nevent>=max_event+skip_event)&&(max_event!=0))){printf("break1\n");break;}
    }
    decreaseIndent();

    if(status!=EOF) printf("\n   *** error reading file, status is: 0x%x ***\n\n",status);

    /* done */
    evio_xmldump_done(xml,maxbuf*sizeof(unsigned int)*EVIO2XML);
    writeit(out,xml,strlen(xml));

    sprintf(s,"</%s>\n\n",main_tag);
    writeit(out,s,strlen(s));
    evClose(handle);
#ifndef _MSC_VER
    if((out!=NULL)&&(gzip!=0))gzclose(out);
#endif
    exit(EXIT_SUCCESS);
}


/*---------------------------------------------------------------- */


void writeit(FILE *f, char *s, int len) {

    if(f==NULL) {
        printf("%s",s);
    } else if (gzip==0) {
        fprintf(f,s,len);
#ifndef _MSC_VER
    } else {
        gzwrite(f,s,len);
#endif
    }

}


/*---------------------------------------------------------------- */


int user_event_select(unsigned int *buf) {

    int i;
    int event_tag = buf[1]>>16;


    if((nevok<=0)&&(nnoev<=0)) {
        return(1);

    } else if(nevok>0) {
        for(i=0; i<nevok; i++) if(event_tag==evok[i])return(1);
        return(0);

    } else {
        for(i=0; i<nnoev; i++) if(event_tag==noev[i])return(0);
        return(1);
    }

}


/*---------------------------------------------------------------- */


int user_frag_select(int tag) {

    int i;

    if((nfragok<=0)&&(nnofrag<=0)) {
        return(1);

    } else if(nfragok>0) {
        for(i=0; i<nfragok; i++) if(tag==fragok[i])return(1);
        return(0);

    } else {
        for(i=0; i<nnofrag; i++) if(tag==nofrag[i])return(0);
        return(1);
    }

}


/*---------------------------------------------------------------- */


void decode_command_line(int argc, char**argv) {

    const char *help =
            "\nusage:\n\n  evio2xml [-max max_event] [-pause] [-skip skip_event]\n"
                    "           [-dict dictfilename] [-dtag dtag]\n"
                    "           [-ev evtag] [-noev evtag] [-frag frag] [-nofrag frag] [-max_depth max_depth]\n"
                    "           [-n8 n8] [-n16 n16] [-n32 n32] [-n64 n64]\n"
                    "           [-verbose] [-brief] [-no_data] [-xtod] [-m main_tag] [-e event_tag]\n"
                    "           [-indent indent_size] [-no_typename] [-maxbuf maxbuf] [-debug]\n"
                    "           [-out outfilenema] [-gz] filename\n";
    int i;


    if(argc<2) {
        printf("%s\n",help);
        exit(EXIT_SUCCESS);
    }


    /* loop over arguments */
    i=1;
    while (i<argc) {
        if (strncasecmp(argv[i],"-h",2)==0) {
            printf("%s\n",help);
            exit(EXIT_SUCCESS);

        } else if (strncasecmp(argv[i],"-pause",6)==0) {
            pause=1;
            i=i+1;

        } else if (strncasecmp(argv[i],"-out",4)==0) {
            outfilename=strdup(argv[i+1]);
            i=i+2;

        } else if (strncasecmp(argv[i],"-maxbuf",7)==0) {
            maxbuf=atoi(argv[i+1]);
            i=i+2;

        } else if (strncasecmp(argv[i],"-debug",6)==0) {
            debug=1;
            i=i+1;

#ifndef _MSC_VER
        } else if (strncasecmp(argv[i],"-gz",3)==0) {
            gzip=1;
            i=i+1;
#endif

        } else if (strncasecmp(argv[i],"-verbose",8)==0) {
            set_verbose(1);
            i=i+1;

        } else if (strncasecmp(argv[i],"-brief",6)==0) {
            set_brief(1);
            i=i+1;

        } else if (strncasecmp(argv[i],"-no_data",8)==0) {
            set_no_data(1);
            i=i+1;

        } else if (strncasecmp(argv[i],"-no_typename",12)==0) {
            set_no_typename(1);
            i=i+1;

        } else if (strncasecmp(argv[i],"-max_depth",10)==0) {
            set_max_depth(atoi(argv[i+1]));
            i=i+2;

        } else if (strncasecmp(argv[i],"-max",4)==0) {
            max_event=atoi(argv[i+1]);
            i=i+2;

        } else if (strncasecmp(argv[i],"-skip",5)==0) {
            skip_event=atoi(argv[i+1]);
            i=i+2;

        } else if (strncasecmp(argv[i],"-dict",5)==0) {
            dictfilename=strdup(argv[i+1]);
            i=i+2;

        } else if (strncasecmp(argv[i],"-dtag",5)==0) {
            dicttagname=strdup(argv[i+1]);
            i=i+2;

        } else if (strncasecmp(argv[i],"-xtod",5)==0) {
            set_xtod(1);
            i=i+1;

        } else if (strncasecmp(argv[i],"-ev",3)==0) {
            if(nevok<(sizeof(evok)/sizeof(int))) {
                evok[nevok++]=atoi(argv[i+1]);
                i=i+2;
            } else {
                printf("?too many ev flags: %s\n",argv[i+1]);
            }

        } else if (strncasecmp(argv[i],"-noev",5)==0) {
            if(nnoev<(sizeof(noev)/sizeof(int))) {
                noev[nnoev++]=atoi(argv[i+1]);
                i=i+2;
            } else {
                printf("?too many noev flags: %s\n",argv[i+1]);
            }

        } else if (strncasecmp(argv[i],"-frag",5)==0) {
            if(nfragok<(sizeof(fragok)/sizeof(int))) {
                fragok[nfragok++]=atoi(argv[i+1]);
                i=i+2;
            } else {
                printf("?too many frag flags: %s\n",argv[i+1]);
            }

        } else if (strncasecmp(argv[i],"-nofrag",7)==0) {
            if(nnofrag<(sizeof(nofrag)/sizeof(int))) {
                nofrag[nnofrag++]=atoi(argv[i+1]);
                i=i+2;
            } else {
                printf("?too many nofrag flags: %s\n",argv[i+1]);
            }

        } else if (strncasecmp(argv[i],"-n8",3)==0) {
            set_n8(atoi(argv[i+1]));
            i=i+2;

        } else if (strncasecmp(argv[i],"-n16",4)==0) {
            set_n16(atoi(argv[i+1]));
            i=i+2;

        } else if (strncasecmp(argv[i],"-n32",4)==0) {
            set_n32(atoi(argv[i+1]));
            i=i+2;

        } else if (strncasecmp(argv[i],"-n64",4)==0) {
            set_n64(atoi(argv[i+1]));
            i=i+2;

        } else if (strncasecmp(argv[i],"-m",2)==0) {
            main_tag=argv[i+1];
            i=i+2;

        } else if (strncasecmp(argv[i],"-e",2)==0) {
            set_event_tag(strdup(argv[i+1]));
            i=i+2;

        } else if (strncasecmp(argv[i],"-",1)==0) {
            printf("\n  ?unknown command line arg: %s\n\n",argv[i]);
            exit(EXIT_FAILURE);

        } else {
            break;
        }
    }

    /* last arg better be filename */
    filename=argv[argc-1];

    return;
}


/*----------------------------------------------------------------
 * Start of what was originally xml_util.c
 *----------------------------------------------------------------*/


void evio_xmldump_init(char *dictfilename, char *dtagname) {

    dicttagname2=strdup(dtagname);
    create_dictionary(dictfilename);

    return;
}


/*---------------------------------------------------------------- */


static void create_dictionary(char *dictfilename) {

    FILE *dictfile;
    int len;


    /* is dictionary file specified */
    if(dictfilename==NULL)return;


    /* read add'l tags from xml file */
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


static void startDictElement(void *userData, const char *name, const char **atts) {

    int natt=XML_GetSpecifiedAttributeCount(dictParser);
    char *tagtext,*p,*numtext;
    const char *cp;
    int i,nt,nn;
    int *ip, *in;


    if(strcasecmp(name,dicttagname2)!=0)return;

    ndict++;
    if(ndict>MAXDICT) {
        printf("\n?too many dictionary entries in file\n\n");
        exit(EXIT_FAILURE);
    }


    /* store tags */
    nt=0;
    ip=NULL;
    cp=get_char_att(atts,natt,"tag");
    if(cp!=NULL) {
        nt=1;
        tagtext=strdup(cp);
        for(i=0; i<strlen(tagtext); i++) if(tagtext[i]=='.')nt++;
        ip=(int*)malloc(nt*sizeof(int));

        i=0;
        p=tagtext-1;
        do {
            ip[i++]=atoi(++p);
            p=strchr(p,'.');
        } while (p!=NULL);
    }


    /* store nums */
    nn=0;
    in=NULL;
    cp=get_char_att(atts,natt,"num");
    if(cp!=NULL) {
        nn=1;
        numtext=strdup(cp);
        for(i=0; i<strlen(numtext); i++) if(numtext[i]=='.')nn++;
        in=(int*)malloc(nn*sizeof(int));

        i=0;
        p=numtext-1;
        do {
            in[i++]=atoi(++p);
            p=strchr(p,'.');
        } while (p!=NULL);
    }


    /* store dictionary info */
    dict[ndict-1].name = strdup(get_char_att(atts,natt,"name"));
    dict[ndict-1].ntag = nt;
    dict[ndict-1].tag  = ip;
    dict[ndict-1].nnum = nn;
    dict[ndict-1].num  = in;

    return;
}



/**
 * This routine creates an xml representation of an evio event.
 *
 * @param buf    buffer with evio event data
 * @param bufnum buffer id number
 * @param string buffer in which to place the resulting xml
 * @param len    length of xml string buffer
 */
void evio_xmldump(unsigned int *buf, int bufnum, char *string, int len) {

    nbuf=bufnum;
    xml=string;

    indent();

    xml+=sprintf(xml,"<!-- ===================== Buffer %d contains %d words (%d bytes) "
            "===================== -->\n",nbuf,buf[0]+1,4*(buf[0]+1));

    depth=0;
    decreaseIndent();
    dump_fragment(buf,BANK);


    return;
}


/*---------------------------------------------------------------- */


void set_user_frag_select_func( int (*f) (int tag) ) {
    USER_FRAG_SELECT_FUNC = f;
    return;
}



/**
 * This routine puts evio container data into an xml string representing an evio event.
 *
 * @param buf            pointer to data to put in xml string
 * @param fragment_type  container type of evio data (bank=0, segment=1, or tagsegment=2)
 */
static void dump_fragment(unsigned int *buf, int fragment_type) {

    int i, length,type,is_a_container,noexpand, padding=0;
    unsigned short tag;
    unsigned char num;

    char *myname;


    /* get type-dependent info */
    switch(fragment_type) {
        case BANK:
            length      = buf[0]+1;
            tag         = (buf[1]>>16)&0xffff;
            type        = (buf[1]>>8)&0x3f;
            padding     = (buf[1]>>14)&0x3;
            num         = buf[1]&0xff;
            break;

        case SEGMENT:
            length      = (buf[0]&0xffff)+1;
            type        = (buf[0]>>16)&0x3f;
            padding     = (buf[0]>>22)&0x3;
            tag         = (buf[0]>>24)&0xff;
            num         = -1;  /* doesn't have num */
            break;

        case TAGSEGMENT:
            length      = (buf[0]&0xffff)+1;
            type        = (buf[0]>>16)&0xf;
            tag         = (buf[0]>>20)&0xfff;
            num         = -1;   /* doesn't have num */
            break;

        default:
            printf("?illegal fragment_type in dump_fragment: %d",fragment_type);
            exit(EXIT_FAILURE);
            break;
    }


    /* user selection on fragment tags (not on event tag) */
    if( (depth>0) && (USER_FRAG_SELECT_FUNC != NULL) && (USER_FRAG_SELECT_FUNC(tag)==0) )return;


    /* update depth, tagstack, numstack, etc. */
    depth++;
    if( (depth>(sizeof(tagstack)/sizeof(int))) || (depth>(sizeof(numstack)/sizeof(int))) ) {
        printf("?error...tagstack/numstack overflow\n");
        exit(EXIT_FAILURE);
    }
    tagstack[depth-1]=tag;
    numstack[depth-1]=num;
    is_a_container=evIsContainer(type);
    myname=(char*)get_matchname();
    noexpand=is_a_container&&(max_depth>=0)&&(depth>max_depth);


    /* xml opening fragment */
    increaseIndent();


    /* verbose header */
    if(verbose!=0) {
        xml+=sprintf(xml,"\n"); indent();
        if(fragment_type==BANK) {
            xml+=sprintf(xml,"<!-- header words: %d, %#x -->\n",buf[0],buf[1]);
        } else {
            xml+=sprintf(xml,"<!-- header word: %#x -->\n",buf[0]);
        }
    }


    indent();

    /* format and content */
    if((fragment_type==BANK)&&(depth==1)) {
        xml+=sprintf(xml,"<%s format=\"evio\" count=\"%d\"",event_tag,nbuf);
        if(brief==0)xml+=sprintf(xml," content=\"%s\"",evGetTypename(type));
    } else if(myname!=NULL) {
        xml+=sprintf(xml,"<%s",myname);
        if(brief==0)xml+=sprintf(xml," content=\"%s\"",evGetTypename(type));
    } else if((fragment_type==BANK)&&(depth==2)) {
        xml+=sprintf(xml,"<%s",bank2_tag);
        if(brief==0)xml+=sprintf(xml," content=\"%s\"",evGetTypename(type));
    } else if(is_a_container||no_typename) {
        xml+=sprintf(xml,"<%s",fragment_name[fragment_type]);
        if(brief==0)xml+=sprintf(xml," content=\"%s\"",evGetTypename(type));
    } else {
        xml+=sprintf(xml,"<%s",evGetTypename(type));
    }

    /* data_type, tag, and num */
    if(brief==0)xml+=sprintf(xml," data_type=\"0x%x\"",type);
    if(brief==0)xml+=sprintf(xml," tag=\"%d\"",tag);
    if(brief==0)xml+=sprintf(xml," padding=\"%d\"",padding);
    if((brief==0)&&(fragment_type==BANK))xml+=sprintf(xml," num=\"%d\"",(int)num);

    /* length, ndata for verbose */
    /*sergey if(verbose!=0) replaced by*/if(brief==0) {
        xml+=sprintf(xml," length=\"%d\" ndata=\"%d\"", (length-1),
                     get_ndata(type, (length - fragment_offset[fragment_type]), padding));
    }


    /* noexpand option */
    if(noexpand)xml+=sprintf(xml," opt=\"noexpand\"");
    xml+=sprintf(xml,">\n");


    /* fragment data */
    dump_data(&buf[fragment_offset[fragment_type]], type,
              length-fragment_offset[fragment_type], padding, noexpand);


    /* xml closing fragment */
    indent();
    if((fragment_type==BANK)&&(depth==1)) {
        xml+=sprintf(xml,"</%s>\n\n",event_tag);
        indent();
        xml+=sprintf(xml,"<!-- end buffer %d -->\n\n",nbuf);
    } else if(myname!=NULL) {
        xml+=sprintf(xml,"</%s>\n",myname);
    } else if((fragment_type==BANK)&&(depth==2)) {
        xml+=sprintf(xml,"</%s>\n",bank2_tag);
    } else if(is_a_container||no_typename) {
        xml+=sprintf(xml,"</%s>\n",fragment_name[fragment_type]);
    } else {
        xml+=sprintf(xml,"</%s>\n",evGetTypename(type));
    }

    decreaseIndent();

    /* decrement stack depth */
    depth--;


    return;
}


/*---------------------------------------------------------------- */

/**
 * This routine represents the contents of a single composite data type
 * in the xml string.
 *
 * @param buf buffer with composite data
 * @return the number of words in single composite item's evio representation.
 */
static int dump_composite(unsigned int *buf) {

    int i, length, type, is_a_container, noexpand, compLen=1;
    unsigned short tag;
    unsigned char num;

    int padding;
    int index = 1;
    int nfmt;
    int sLen;
    char fmt[256], *ch;
    unsigned char ifmt[256];
    int ifmtlength = 255;


    /* get format info from tag fragment */
    length  	= (buf[0]&0xffff)+1;
    type    	= (buf[0]>>16)&0xf;
    tag     	= (buf[0]>>20)&0xfff;
    num     	= -1;   /* doesn't have num */

    /*
    printf("1---> %d %d %d %d\n",length,type,tag,num);
    */


    /* update depth, tagstack, numstack, etc. */
    depth++;
    is_a_container = evIsContainer(type);
    noexpand = is_a_container && (max_depth >= 0) && (depth > max_depth);

    /* verbose header */
    if(verbose!=0) {
        xml += sprintf(xml,"\n");
        indent();
        xml += sprintf(xml, "<!-- header word: %#x -->\n",buf[0]);
    }


    /* signify start of array element */
    indent();
    xml += sprintf(xml, "<comp>\n");


    /* format and content */
    increaseAndIndent();
    xml += sprintf(xml, "<format ");


    /* data_type, tag, and num */
    if (brief==0) xml += sprintf(xml," data_type=\"0x%x\"",type);
    if (brief==0) xml += sprintf(xml," tag=\"%d\"",tag);


    /* length, ndata */
    xml += sprintf(xml, " length=\"%d\" ndata=\"%d\"", (length-1), get_ndata(type,length-1,0));

    /* noexpand option */
    if (noexpand) xml += sprintf(xml," opt=\"noexpand\"");
    xml += sprintf(xml, ">\n");


    /* dump format string. Just one string, ends on null so simple */
    /* tag contains the number of characters in format string - Sergey */
    /*TODO: This was never mentioned to me - Carl */
    increaseAndIndent();
    xml += sprintf(xml,"%s\n", (char*)&buf[1]);


    /* xml closing fragment */
    decreaseAndIndent();
    xml += sprintf(xml,"</format>\n");


    /* decrement stack depth */
    depth--;


    /* dump data using format */
    ch = (char*)&buf[index];
    sLen = strlen(ch);
    strncpy(fmt,ch, sLen);
    fmt[sLen] = '\0';

    nfmt = eviofmt(fmt, ifmt, ifmtlength);
/*printf("format string = %s, nfmt = %d\n",fmt, nfmt);*/

    /* skip the rest of tagsegment */
    index += (length-1);


    length  =  buf[index] + 1;
    tag     = (buf[index+1] >> 16) & 0xffff;
    type    = (buf[index+1] >> 8) & 0x3f;
    num     =  buf[index+1] & 0xff;
    padding = (buf[index+1] >> 14) & 0x3;


    /* skip bank header */
    index  += 2;
    length -= 2;
    compLen = index + length;

/*printf("length = %d words, padding = %d extra bytes\n",length, padding);*/

    indent();
    xml += sprintf(xml,"<data  tag=\"%d\" num=\"%d\">\n",tag, num);
    xml += eviofmtdump((int *)&buf[index], length, ifmt, nfmt, padding, getIndent(), !xtod, xml);
    indent();
    xml += sprintf(xml,"</data>\n");

    decreaseAndIndent();
    xml+=sprintf(xml,"</comp>\n");

    return compLen;
}



/**
 * This routine puts evio data into an xml string representing an evio event.
 *
 * @param data     pointer to data to put in xml string
 * @param type     type of evio data (ie. short, bank, etc)
 * @param length   length of data in 32 bit words
 * @param padding  number of bytes to be ignored at end of data
 * @param noexpand if true, just print data as ints, don't expand as detailed xml
 */
static void dump_data(unsigned int *data, int type, int length, int padding, int noexpand) {

    int i,j,len, compLen=0;
    int p=0;
    short *s;
    char *c, *start;
    unsigned char *uc;
    char format[132];

    /* dump data if no expansion, even if this is a container */
    if (noexpand) {
        if (xtod) {
            sprintf(format,"%%11d  ");
        }
        else {
            sprintf(format,"0x%%08x  ");
        }

        for(i=0; i<length; i+=n32) {
            indent();
            for(j=i; j<min((i+n32),length); j++) {
                xml+=sprintf(xml,format,data[j]);
            }
            xml+=sprintf(xml,"\n");
        }
        return;
    }


    /* dump the data or call dump_fragment */
    switch (type) {

        /* unknown */
        case 0x0:

            /* unsigned 32 bit int */
        case 0x1:
            if(!no_data) {
                increaseIndent();
                if (xtod) {
                    sprintf(format,"%%11u  ");
                }
                else {
                    sprintf(format,"0x%%08x  ");
                }
                for(i=0; i<length; i+=n32) {
                    indent();
                    for(j=i; j<min((i+n32),length); j++) {
                        xml+=sprintf(xml,format,data[j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* 32 bit float */
        case 0x2:
            if(!no_data) {
                increaseIndent();
                sprintf(format,"%%#15.8g  ");
                for(i=0; i<length; i+=n32) {
                    indent();
                    for(j=i; j<min(i+n32,length); j++) {
                        xml+=sprintf(xml,format,*(float*)&data[j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* string */ // TODO: need to print array of strings
        case 0x3:
            if(!no_data) {
                increaseIndent();
                start=(char*)&data[0];
                c=start;
                while((c[0]!=0x4)&&((c-start)<length*4)) {
                    len=strlen(c);
                    indent();
                    sprintf(format,"<![CDATA[%%.%ds]]>\n",len);
                    xml+=sprintf(xml,format,c);
                    c+=len+1;
                }
                decreaseIndent();
            }
            break;

            /* 16 bit int */
        case 0x4:
            if(!no_data) {
                int numShorts = 2*length;
                increaseIndent();
                if (padding == 2) numShorts--;

                if (xtod) {
                    sprintf(format,"%%6hd  ");
                }
                else {
                    sprintf(format,"0x%%04hx  ");
                }
                s=(short*)&data[0];
                for(i=0; i<numShorts; i+=n16) {
                    indent();
                    for(j=i; j<min(i+n16,numShorts); j++) {
                        xml+=sprintf(xml,format,s[j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* unsigned 16 bit int */
        case 0x5:
            if(!no_data) {
                int numShorts = 2*length;
                increaseIndent();
                if (padding == 2) numShorts--;

                if (xtod) {
                    sprintf(format,"%%6hu  ");
                }
                else {
                    sprintf(format,"0x%%04hx  ");
                }
                s=(short*)&data[0];
                for(i=0; i<numShorts; i+=n16) {
                    indent();
                    for(j=i; j<min(i+n16,numShorts); j++) {
                        xml+=sprintf(xml,format,s[j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* 8 bit int */
        case 0x6:
            if(!no_data) {
                int numBytes = 4*length;
                increaseIndent();
                if (padding >=1 && padding <= 3) numBytes -= padding;

                if (xtod) {
                    sprintf(format,"%%4d  ");
                }
                else {
                    sprintf(format,"0x%%02hhx  ");
                }
                c=(char*)&data[0];
                for(i=0; i<numBytes; i+=n8) {
                    indent();
                    for(j=i; j<min(i+n8,numBytes); j++) {
                        xml+=sprintf(xml,format,c[j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* unsigned 8 bit int */
        case 0x7:
            if(!no_data) {
                int numBytes = 4*length;
                increaseIndent();
                if (padding >=1 && padding <= 3) numBytes -= padding;

                if (xtod) {
                    sprintf(format,"%%4u  ");
                }
                else {
                    sprintf(format,"0x%%02hhx  ");
                }
                uc=(unsigned char*)&data[0];
                for(i=0; i<numBytes; i+=n8) {
                    indent();
                    for(j=i; j<min(i+n8,numBytes); j++) {
                        xml+=sprintf(xml,format,uc[j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* 64 bit double */
        case 0x8:
            if(!no_data) {
                increaseIndent();
                sprintf(format,"%%#25.17g  ");
                for(i=0; i<length/2; i+=n64) {
                    indent();
                    for(j=i; j<min(i+n64,length/2); j++) {
                        xml+=sprintf(xml,format,*(double*)&data[2*j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* 64 bit int */
        case 0x9:
            if(!no_data) {
                increaseIndent();
                if (xtod) {
                    sprintf(format,"%%20lld  ");
                }
                else {
                    sprintf(format,"0x%%016llx  ");
                }
                for(i=0; i<length/2; i+=n64) {
                    indent();
                    for(j=i; j<min(i+n64,length/2); j++) {
                        xml+=sprintf(xml,format,*(int64_t *)&data[2*j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* unsigned 64 bit int */
        case 0xa:
            if(!no_data) {
                increaseIndent();
                if (xtod) {
                    sprintf(format,"%%20llu  ");
                }
                else {
                    sprintf(format,"0x%%016llx  ");
                }
                for(i=0; i<length/2; i+=n64) {
                    indent();
                    for(j=i; j<min(i+n64,length/2); j++) {
                        xml+=sprintf(xml,format,*(uint64_t *)&data[2*j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* 32 bit int */
        case 0xb:
            if(!no_data) {
                increaseIndent();
                if (xtod) {
                    sprintf(format,"%%11d  ");
                }
                else {
                    sprintf(format,"0x%%08x  ");
                }
                for(i=0; i<length; i+=n32) {
                    indent();
                    for(j=i; j<min((i+n32),length); j++) {
                        xml+=sprintf(xml,format,data[j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;

            /* composite array */
        case 0xf:
            if(!no_data) {
                while (compLen < length) {
                    increaseIndent();
                    compLen += dump_composite(&data[compLen]);
                    decreaseIndent();
                }
            }
            break;

            /* bank */
        case 0xe:
        case 0x10:
            while(p<length) {
                dump_fragment(&data[p],BANK);
                p+=data[p]+1;
            }
            break;

            /* segment */
        case 0xd:
        case 0x20:
            while(p<length) {
                dump_fragment(&data[p],SEGMENT);
                p+=(data[p]&0xffff)+1;
            }
            break;

            /* tagsegment */
        case 0xc:
            while(p<length) {
                dump_fragment(&data[p],TAGSEGMENT);
                p+=(data[p]&0xffff)+1;
            }
            break;

        default:
            if(!no_data) {
                increaseIndent();
                if (xtod) {
                    sprintf(format,"%%11u  ");
                }
                else {
                    sprintf(format,"0x%%08x  ");
                }

                for(i=0; i<length; i+=n32) {
                    indent();
                    for(j=i; j<min(i+n32,length); j++) {
                        xml+=sprintf(xml,format,(unsigned int)data[j]);
                    }
                    xml+=sprintf(xml,"\n");
                }
                decreaseIndent();
            }
            break;
    }

    return;
}


/*---------------------------------------------------------------- */

/**
 * Get the number of items given the data type, data length, and padding.
 *
 * @param type    numerical value of data type
 * @param length  length of data in 32 bit words
 * @param padding number of bytes used to pad data at the end:
 *                 0 or 2 for short types, 0-3 for byte types
 */
static int get_ndata(int type, int length, int padding) {

    switch (type) {

        case 0x0:
        case 0x1:
        case 0x2:
            return(length);

        case 0x3:
            //??? // TODO: strings arrays?
            return(1);

            /* 16 bit ints */
        case 0x4:
        case 0x5:
            if (padding == 2) {
                return(2*length - 1);
            }
            return(2*length);

            /* 8 bit ints */
        case 0x6:
        case 0x7:
            if (padding >= 0 && padding <= 3) {
                return(4*length - padding);
            }
            return(4*length);

        case 0x8:
        case 0x9:
        case 0xa:
            return(length/2);

        case 0xb:
        case 0xc:
        case 0xd:
        case 0xe:
        case 0x10:
        case 0x20:
        case 0x40:
        default:
            return(length);
    }
}



/*---------------------------------------------------------------- */


static const char *get_char_att(const char **atts, const int natt, const char *name) {

    int i;

    for(i=0; i<natt; i+=2) {
        if(strcasecmp(atts[i],name)==0)return((const char*)atts[i+1]);
    }

    return(NULL);
}


/*---------------------------------------------------------------- */


static const char *get_matchname() {

    int i,j,ntd,nnd,nt,nn,num;
    int tagmatch,nummatch;


    /* search dictionary for tag/num match */
    for(i=0; i<ndict; i++) {

        tagmatch=1;
        ntd=dict[i].ntag;
        if(ntd>0) {
            nt=min(ntd,depth);
            for(j=0; j<nt; j++) {
                if(dict[i].tag[ntd-j-1]!=tagstack[depth-j-1]) {
                    tagmatch=0;
                    break;
                }
            }
        }

        nummatch=1;
        nnd=dict[i].nnum;
        if(nnd>0) {
            nn=min(nnd,depth);
            for(j=0; j<nn; j++) {
                num=numstack[depth-j-1];
                if((num>=0)&&(dict[i].num[nnd-j-1]!=num)) {
                    nummatch=0;
                    break;
                }
            }
        }

        /* tag and num match, done */
        if((tagmatch==1)&&(nummatch==1))return(dict[i].name);

    }  /* try next dictionary element */

    return(NULL);
}


/*---------------------------------------------------------------- */
void evio_xmldump_done(char *string, int len) {
    sprintf(string," ");
    return;
}


/*---------------------------------------------------------------- */
/*--- Set functions ---------------------------------------------- */
/*---------------------------------------------------------------- */


int set_event_tag(char *tag) {
    event_tag=tag;
    return(0);
}

/*---------------------------------------------------------------- */
int set_bank2_tag(char *tag) {
    bank2_tag=tag;
    return(0);
}

/*---------------------------------------------------------------- */
int set_n8(int val) {
    n8=val;
    return(0);
}

/*---------------------------------------------------------------- */
int set_n16(int val) {
    n16=val;
    return(0);
}

/*---------------------------------------------------------------- */
int set_n32(int val) {
    n32=val;
    return(0);
}

/*---------------------------------------------------------------- */
int set_n64(int val) {
    n64=val;
    return(0);
}

/*---------------------------------------------------------------- */
int set_xtod(int val) {
    xtod=val;
    return(0);
}

/*---------------------------------------------------------------- */
int set_max_depth(int val) {
    max_depth=val;
    return(0);
}

/*---------------------------------------------------------------- */
int set_no_typename(int val) {
    no_typename=val;
    return(0);
}

/*---------------------------------------------------------------- */
int set_verbose(int val) {
    verbose=val;
    return(0);
}

/*---------------------------------------------------------------- */
int set_brief(int val) {
    brief=val;
    return(0);
}

/*---------------------------------------------------------------- */
int set_no_data(int val) {
    no_data=val;
    return(0);
}

/*---------------------------------------------------------------- */

