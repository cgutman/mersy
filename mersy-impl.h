#ifndef _MERSY_IMPL_H
#define _MERSY_IMPL_H

// Specifies the current trace level
extern int TraceLevel;

// Implementation configuration options
#define DIVISIONS_FOR_PRIMALITY (1)

// Macro used for printing messages
#define PRINT_MSG(x, ...)                       \
	if (x >= TraceLevel) {                  \
		PRINT_MSG_HEADER(x);            \
		PRINT_MSG_BODY(x, __VA_ARGS__); \
       		PRINT_MSG_FOOTER(x);            \
	}

// Prints the header before each message. The default implementation
// prints the date and time.
void PrintMsgHeader(int tag);
#define PRINT_MSG_HEADER(x) PrintMsgHeader(x)

// Prints the message body. The default implementation calls printf().
#define PRINT_MSG_BODY(x, ...) printf(__VA_ARGS__)

// Prints the footer. The default implementation prints a newline
// and flushes the output stream.
#define PRINT_MSG_FOOTER(x) \
	printf("\n");       \
	fflush(stdout)

#endif

