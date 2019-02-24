#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

/*
Each record is like this:

4 bytes: latitude * 1000000
4 bytes: longitude * 1000000
4 bytes: altitude
4 bytes: speed * 100
4 bytes: distance interval * 100
4 bytes: time interval * 10
1 bytes: status; points whose status is 2 or 3 are bad
1 bytes: heart rate
22 bytes: unknown
*/
struct Record {
    int latitude;
    int longitude;
    int altitude;
    int speed;
    int distance_interval;
    int time_interval;
    unsigned char status;
    unsigned char heart_rate;
    unsigned char unknown[22];
};

long mktrkpt(const struct Record* r, xmlNodePtr parent, long baseTime);
const char* xstrftimel(const char* format, const long);
const char* xstrftime(const char* format, const struct tm* time);

int read_record(FILE *f, struct Record *r) {
    return fread(r, sizeof(struct Record), 1, f);
}

int cpw_to_gpx(const char* inputFile, const char* outputFile) {
    FILE *inputf = fopen(inputFile, "r");
    FILE *outputf = fopen(outputFile, "w+");

    struct tm initial_time;

    char fileNameBuf[4096];
    if (!strncpy(fileNameBuf, inputFile, 4096)) {
        return 1;
    }
    // 2018-09-09-16'53'09.cpw
    strstr(fileNameBuf, ".")[0] = '\0';
    printf("%s %s\n", inputFile, fileNameBuf);

    if (!strptime(fileNameBuf, "%Y-%m-%d-%H'%M'%S", &initial_time)) {
        time_t now = time(0);
        initial_time = *gmtime(&now);
    }

    xmlNodePtr gpxNode = xmlNewNode(NULL, BAD_CAST "gpx");
    xmlNodePtr trkNode = xmlNewChild(gpxNode, NULL, BAD_CAST "trk", BAD_CAST "");
    xmlNodePtr segNode = xmlNewChild(trkNode, NULL, BAD_CAST "trkseg", BAD_CAST "");

    time_t initital_t = mktime(&initial_time);
    long timeAccumulatorX10 = initital_t * 10;

    struct Record r;
    for (int i = read_record(inputf, &r); i > 0; i = read_record(inputf, &r)) {
        timeAccumulatorX10 += mktrkpt(&r, segNode, timeAccumulatorX10);

        // last row seems different
        if (r.unknown[0] == 0)
            break;

        printf("(%f %f), a=%d, s=%f, d=%f, t=%f, S=%d, h=%d T=%s, u=%d\n", r.latitude/1000000.0, r.longitude/1000000.0, r.altitude, r.speed/100.0, r.distance_interval/100.0, r.time_interval/10.0, r.status, r.heart_rate, xstrftimel("%Y-%m-%dT%H:%M:%SZ", timeAccumulatorX10/10), (char)r.unknown[0]);
    }

    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlDocSetRootElement(doc, gpxNode);
    xmlSaveFile(outputFile, doc);

    if (inputf != NULL)
        fclose(inputf);
    if (outputf != NULL)
        fclose(outputf);

    return 0;
}

char XPRINTFBUFF[4096];
const char* xsprintf(const char* format, ...) {

    va_list arglist;
    va_start(arglist, format);
    vsprintf(XPRINTFBUFF, format, arglist);
    va_end(arglist);
    return XPRINTFBUFF;
}

char XSTRFTIMEBUFF[4096];
const char* xstrftime(const char* format, const struct tm* time) {
    if (!strftime(XSTRFTIMEBUFF, sizeof(XSTRFTIMEBUFF)/ sizeof(XSTRFTIMEBUFF[0]), format, time))
        return NULL;
    return XSTRFTIMEBUFF;
}
const char* xstrftimel(const char* format, const long time) {
    struct tm time_tm = *gmtime(&time);
    return xstrftime(format, &time_tm);
}


long mktrkpt(const struct Record* r, xmlNodePtr parent, long baseTimeX10) {
    xmlNodePtr ptNode = xmlNewChild(parent, NULL, BAD_CAST "trkpt", BAD_CAST "");
    xmlNodePtr eleNode = xmlNewChild(ptNode, NULL, BAD_CAST "ele", BAD_CAST xsprintf("%d", r->altitude));
    xmlNodePtr timeNode = xmlNewChild(ptNode, NULL, BAD_CAST "time", BAD_CAST xstrftimel("%Y-%m-%dT%H:%M:%SZ", (baseTimeX10 + r->time_interval) / 10));
    xmlNewProp(ptNode, BAD_CAST "lat", BAD_CAST xsprintf("%f", r->latitude/1000000.0));
    xmlNewProp(ptNode, BAD_CAST "lon", BAD_CAST xsprintf("%f", r->longitude/1000000.0));
    return r->time_interval;
}

xmlXPathObjectPtr execute_xpath_expression(const xmlDocPtr doc, const xmlChar* xpathExpr) {
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;

    /* Create xpath evaluation context */
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        fprintf(stderr,"Error: unable to create new XPath context\n");
        xmlFreeDoc(doc);
        return(NULL);
    }

    /* Evaluate xpath expression */
    xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
    if(xpathObj == NULL) {
        fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return(NULL);
    }

    xmlXPathFreeContext(xpathCtx);

    return(xpathObj);
}

int main(int argc, const char *argv[]) {
    const char *inFile = argv[1];
    const char *outFile = argv[1];

    exit(cpw_to_gpx(inFile, outFile));
}
