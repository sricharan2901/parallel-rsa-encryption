// File: receiver.c
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "rsa_omp.h"

// RSA Keys (For demonstration purposes; in practice, use secure key generation and storage)
const uint64_t PUBLIC_KEY_E = 65537;
const uint64_t PUBLIC_KEY_N = 3233; // Example small modulus (replace with a large one)
const uint64_t PRIVATE_KEY_D = 2753;

// GUI Widgets
GtkWidget *text_view;

// Server Configuration
int SERVER_PORT = 5001;

// Function to update the text view
void update_text_view(const char *message) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, message, -1);
}

// Function to decrypt the file
int decrypt_file(const char *input_path, const char *output_path, uint64_t d, uint64_t n, char *decryption_info) {
    FILE *fin = fopen(input_path, "rb");
    if (!fin) {
        perror("fopen");
        return -1;
    }

    FILE *fout = fopen(output_path, "wb");
    if (!fout) {
        perror("fopen");
        fclose(fin);
        return -1;
    }

    // Read encrypted bytes and decrypt each
    uint64_t encrypted_byte;
    uint64_t decrypted_byte;
    while (fread(&encrypted_byte, sizeof(encrypted_byte), 1, fin) == 1) {
        decrypted_byte = modular_exponentiation_openmp(encrypted_byte, d, n);
        fputc((unsigned char)decrypted_byte, fout);
    }

    fclose(fin);
    fclose(fout);

    // Prepare decryption info
    sprintf(decryption_info, "Decryption:\nPrivate Key (d, n): (%llu, %llu)\n", d, n);

    return 0;
}

// Function to handle incoming connections
void *handle_client(void *arg) {
    int server_fd = *((int *)arg);
    free(arg);

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket;

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        pthread_exit(NULL);
    }

    // Update GUI
    update_text_view("Accepted new connection.\n");

    // Receive file size
    long filesize_net;
    if (recv(new_socket, &filesize_net, sizeof(filesize_net), 0) <= 0) {
        perror("recv");
        close(new_socket);
        pthread_exit(NULL);
    }
    long filesize = ntohl(filesize_net);

    // Receive file data
    char *buffer = malloc(filesize);
    if (!buffer) {
        perror("malloc");
        close(new_socket);
        pthread_exit(NULL);
    }

    long total_received = 0;
    while (total_received < filesize) {
        ssize_t bytes = recv(new_socket, buffer + total_received, filesize - total_received, 0);
        if (bytes <= 0) {
            perror("recv");
            free(buffer);
            close(new_socket);
            pthread_exit(NULL);
        }
        total_received += bytes;
    }

    // Save the encrypted file
    const char *encrypted_file = "received_file.enc";
    FILE *f = fopen(encrypted_file, "wb");
    if (!f) {
        perror("fopen");
        free(buffer);
        close(new_socket);
        pthread_exit(NULL);
    }
    fwrite(buffer, 1, filesize, f);
    fclose(f);
    free(buffer);
    close(new_socket);

    // Update GUI
    update_text_view("Encrypted file received.\n");

    // Decrypt the file
    char decrypted_file[] = "received_file_decrypted.txt";
    char decryption_info[512] = {0};
    if (decrypt_file(encrypted_file, decrypted_file, PRIVATE_KEY_D, PUBLIC_KEY_N, decryption_info) != 0) {
        update_text_view("Decryption failed.\n");
        pthread_exit(NULL);
    }

    update_text_view(decryption_info);
    update_text_view("File decrypted and saved as 'received_file_decrypted.txt'.\n");

    pthread_exit(NULL);
}

// Function to start the server
void start_server(GtkWidget *widget, gpointer data) {
    pthread_t thread_id;
    int *server_fd = malloc(sizeof(int));
    if (!server_fd) {
        perror("malloc");
        return;
    }

    struct sockaddr_in address;
    int opt = 1;

    // Creating socket file descriptor
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        free(server_fd);
        return;
    }

    // Forcefully attaching socket to the port
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(*server_fd);
        free(server_fd);
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    // Bind the socket to the network address and port
    if (bind(*server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(*server_fd);
        free(server_fd);
        return;
    }

    // Start listening for connections
    if (listen(*server_fd, 3) < 0) {
        perror("listen");
        close(*server_fd);
        free(server_fd);
        return;
    }

    update_text_view("Server started. Waiting for connections...\n");

    // Create a new thread to handle incoming connections
    if (pthread_create(&thread_id, NULL, handle_client, server_fd) != 0) {
        perror("pthread_create");
        close(*server_fd);
        free(server_fd);
        return;
    }

    // Detach the thread so that it cleans up after finishing
    pthread_detach(thread_id);
}

// Main function
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Create main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "RSA File Receiver");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    // Create vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Start Server Button
    GtkWidget *button_start = gtk_button_new_with_label("Start Server");
    g_signal_connect(button_start, "clicked", G_CALLBACK(start_server), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button_start, FALSE, FALSE, 5);

    // Text View for Decryption Info
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 5);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    // Connect the window's destroy signal to gtk_main_quit
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Show all widgets
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
