/* Bageld - A system for the organization,
   storage, and retrieval of Bagel information

   John V. Kew
   Assignment 2
   CPEx317 w/ Dr. Nico
   Winter, 2002

   This program sets up a decision tree for the
   organization of bagel information. The
   program will use a database file in the local
   directory called "bagels.db" - If this file
   does not exist, it will create it so that 
   bagel information can be added.

   Files:
   bageld.c
   bageld.h
   string.c
   string.h

   Compilation: 
   Use cmake

   Usage:
   ./bageld [bagel database] [optional: kerberos keytab]

   Without a database, the program will first
   ask you for a bagel type. Then begin filling
   the database with Caleb bagels, Monkey bagels,
   and Toast bagels.

   All answers are of "yes", "no", [Bagel Name], or
   a question about a bagel.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <gssapi/gssapi_krb5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <signal.h>
#include "bageld.h"
#include "string.h"

#define SUCCESS 0
#define ERROR   1
#define PORT    2002


#include <stdint.h>
#include <stdlib.h>


unsigned char *base64_encode(const unsigned char *data,
                             size_t input_length,
                             size_t *output_length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line

    BIO_write(bio, data, (int) input_length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);
    *output_length = bufferPtr->length;
    unsigned char *buffer = malloc(*output_length);
    memcpy(buffer, (*bufferPtr).data, *output_length);
    free(bufferPtr->data);
    free(bufferPtr);
    return buffer;
}

size_t calcDecodeLength(const unsigned char *b64input) { //Calculates the length of a decoded string
    size_t len = strlen(b64input),
            padding = 0;

    if (b64input[len - 1] == '=' && b64input[len - 2] == '=') //last two chars are =
        padding = 2;
    else if (b64input[len - 1] == '=') //last char is =
        padding = 1;

    return (len * 3) / 4 - padding;
}

unsigned char *base64_decode(const unsigned char *data,
                             size_t input_length,
                             size_t *output_length) {
    BIO *bio, *b64;
    size_t decodeLen = calcDecodeLength(data);
    unsigned char *buffer = (unsigned char *) malloc(decodeLen + 1);
    memset(buffer, '\0', decodeLen);

    bio = BIO_new_mem_buf(data, -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    BIO_read(bio, buffer, (int) input_length);

    *output_length = decodeLen;
    BIO_free_all(bio);

    return buffer;
}

void trim_right(unsigned char *data, size_t len) {
    size_t curr = len - 1;
    while (curr >= 0 && data[curr] <= ' ') {
        data[curr] = '\0';
        curr--;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s database_file [kerberos keytab]", argv[0]);
    }

    printf("Loading Bagel Database... \n");
    for (int i = 0; i < 3; i++) {
        printf("  loading...\n");
        sleep(1);
    }

    struct node *root = readFromFile(argv[1]);
    struct node *curr = root;
    struct node *right = NULL;
    struct node *parent = root;
    struct node *prev = NULL;
    int path;
    size_t inputLength;
    unsigned char *input;
    int finished = 0;

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, 0) == -1) {
        perror("sigaction");
        exit(1);
    }

    /* Bageld net code */
    unsigned int cliLen, newSd, s;
    struct sockaddr_in cliAddr, servAddr;

    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) {
        perror("Cannot open socket ");
        return 1;
    }

    /* bind server port */
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(PORT);

    if (bind(s, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        perror("Cannot bind to port");
        return 1;
    }

    listen(s, 10); /* State a willingness to listen for
                     incoming connections */


    int kerberosEnabled = argc > 2;
    if (kerberosEnabled) {
        if (getenv("KRB5_KTNAME") == NULL) {
            printf("Setting keytab location to %s\n", argv[2]);
            setenv("KRB5_KTNAME", argv[2], 1);
        }
    }

    while (1) {
        printf("Awaiting new connections on port %d\n", PORT);
        cliLen = sizeof(cliAddr);
        newSd = accept(s, (struct sockaddr *) &cliAddr, &cliLen);
        if (newSd < 0) {
            perror("Cannot accept connection ");
            return 1;
        }
        printf("Connect from %s\n", inet_ntoa(cliAddr.sin_addr));

        if (kerberosEnabled) {
            OM_uint32 maj_stat, min_stat, gflags, lmin_s;
            gss_buffer_desc outbuf;
            gss_name_t name;
            int authorized = 0;
            int error = 0;
            do {
                sockwrite("KERBEROS_TICKET:\n", newSd);
                input = sockreadline(&inputLength, newSd);

                printf("KERBEROS: B64Encoded %u [%s]\n", (unsigned int) inputLength, input);
                free(input);

                // Convert from hex to bytes
                size_t ticketLength;
                unsigned char *ticket = base64_decode(input, inputLength, &ticketLength);
                printf("KERBEROS: B64Decoded %u [%s]\n", (unsigned int) ticketLength, ticket);


                gss_buffer_desc gbuf;
                gbuf.length = ticketLength;
                gbuf.value = ticket;
                printf("KERBEROS: Recieved a kerberos service ticket of length %d [%s]\n", gbuf.length, gbuf.value);
                gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;
                maj_stat = gss_accept_sec_context(&min_stat,
                                                  &ctx,
                                                  GSS_C_NO_CREDENTIAL,
                                                  &gbuf,
                                                  GSS_C_NO_CHANNEL_BINDINGS,
                                                  &name,
                                                  NULL,
                                                  &outbuf,
                                                  &gflags,
                                                  NULL,
                                                  NULL);
                printf("KERBEROS: gss_accept_sec_context major: %d, minor: %d, outlen: %u, outflags: %x\n", maj_stat,
                       min_stat, (unsigned int) outbuf.length, gflags);
                free(ticket);
                switch (maj_stat) {
                    case GSS_S_COMPLETE:
                        authorized = 1;
                        gss_buffer_desc   dsp_name;
                        dsp_name.length = 0;
                        dsp_name.value  = NULL;
                        gss_display_name( &min_stat, name, &dsp_name, GSS_C_NO_OID );
                        printf("KERBEROS: accepting GSS security context for: %s\n", (char *)(dsp_name.value));
                        break;
                    case GSS_S_CONTINUE_NEEDED:
                        if (outbuf.length != 0) {
                            // Additional information needs to be sent to the client
                            sockwrite("GSS_S_CONTINUE_NEEDED\n", newSd);
                            printf("KERBEROS: sending GSS response token of length %u [%s]\n",
                                   (unsigned int) outbuf.length, outbuf.value);
                            int b64responseSize = 0;
                            char *b64response = base64_encode(outbuf.value, outbuf.length, &b64responseSize);
                            printf("KERBEROS:  -> Encoded response token of length %u [%s]\n",
                                   (unsigned int) b64responseSize, b64response);
                            sockwrite(b64response, newSd);
                            sockwrite("\n", newSd);
                            gss_release_buffer(&lmin_s, &outbuf);
                        }
                        break;
                    default:
                        gss_delete_sec_context(&lmin_s, &ctx, GSS_C_NO_BUFFER);
                        gss_buffer_desc gmsg;
                        OM_uint32 lmin_s, msg_ctx;
                        char msg_major[128], msg_minor[128];
                        msg_ctx = 0;
                        gss_display_status(&lmin_s, maj_stat, GSS_C_GSS_CODE, GSS_C_NO_OID, &msg_ctx, &gmsg);
                        strlcpy(msg_major, gmsg.value, sizeof(msg_major));
                        gss_release_buffer(&lmin_s, &gmsg);
                        msg_ctx = 0;
                        gss_display_status(&lmin_s, min_stat, GSS_C_MECH_CODE, GSS_C_NO_OID, &msg_ctx, &gmsg);
                        strlcpy(msg_minor, gmsg.value, sizeof(msg_minor));
                        gss_release_buffer(&lmin_s, &gmsg);
                        printf("KERBEROS: accepting GSS security context failed: %s: %s\n", msg_major, msg_minor);
                        error = 1;
                }
            } while (maj_stat == GSS_S_CONTINUE_NEEDED && error == 0);


            if (authorized) {
                sockwrite("Good news; you are not an intruder!", newSd);
            } else {
                sockwrite("Invalid kerberos ticket. You are denied access!", newSd);
                close(newSd);
                continue;
            }
        }

        // Normal Bagled
        finished = 0;
        parent = root;
        curr = root;

        /* Creation of initial bagel in database */
        if (root->left == NULL && root->right == NULL
            && root->questionOrAnswer == NULL) {
            sockwrite("No bagels in database, starting anew.\n", newSd);
            sockwrite("Give the name of a bagel: ", newSd);
            input = sockreadline(&inputLength, newSd);
            trim_right(input, inputLength);
            root->questionOrAnswer = input;
            errno = sockwrite("Thank you.\n", newSd);
            close(newSd);
            finished = !finished;
        }


        while (!finished && errno >= 0) {
            if (curr->left == NULL || curr->right == NULL) { /* At answer node */
                sockwrite("I guess you have a ", newSd);
                sockwrite(curr->questionOrAnswer, newSd);
                sockwrite(" bagel.\n", newSd);
                errno = sockwrite("Is this correct?[yes,no] > ", newSd);
                input = sockreadline(&inputLength, newSd);
                trim_right(input, inputLength);

                if (!strcmp("yes", input)) { /* Congrats message */
                    free(input);
                    finished = !finished;
                    printf("Closing connection...");
                    close(newSd);
                    printf("done\n");
                } else if (!strcmp("no", input)) { /* Create new node */
                    free(input);
                    errno = sockwrite("No? Then what type of bagel is it? > ", newSd);
                    input = sockreadline(&inputLength, newSd);
                    trim_right(input, inputLength);
                    right = createnode();
                    right->questionOrAnswer = input;
                    right->question = 0;
                    right->right = NULL;
                    right->left = NULL;
                    sockwrite("\nWhat is a good yes/no question about ", newSd);
                    sockwrite(input, newSd);
                    sockwrite(" bagels that would distinguish them from ", newSd);
                    sockwrite(curr->questionOrAnswer, newSd);
                    sockwrite(" bagels? > ", newSd);
                    input = sockreadline(&inputLength, newSd);
                    trim_right(input, inputLength);
                    parent = createnode();
                    parent->questionOrAnswer = input;
                    parent->question = 1;
                    parent->right = right;
                    parent->left = curr;
                    if (prev != NULL && path == RIGHT)
                        prev->right = parent;
                    else if (prev != NULL && path == LEFT)
                        prev->left = parent;
                    else if (root == parent->left)
                        root = parent;
                    curr = root;
                    errno = sockwrite("I'm sure I'll guess it now.", newSd);
                    finished = !finished;
                    printf("Closing connection...");
                    close(newSd);
                    printf("done\n");
                } else {
                    free(input);
                    errno = sockwrite("Please answer \"yes\" or \"no\".\n", newSd);
                }

            } else { /* At question node */
                sockwrite(curr->questionOrAnswer, newSd);
                errno = sockwrite(" > ", newSd);
                input = sockreadline(&inputLength, newSd);
                trim_right(input, inputLength);
                if (!strcmp("yes", input)) { /* Goto right node */
                    prev = curr;
                    path = RIGHT;
                    curr = curr->right;
                } else if (!strcmp("no", input)) { /* Goto left node */
                    prev = curr;
                    path = LEFT;
                    curr = curr->left;
                } else
                    errno = sockwrite("Please answer \"yes\" or \"no\".\n", newSd);
                free(input);
            }
        }
        printf("Saving data...");
        saveToFile("bagels.db", root);
        printf("done\n");
        errno = 0;
    }

    destroynode(root);
    return 0;
}
