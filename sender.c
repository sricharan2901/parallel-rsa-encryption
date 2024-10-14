// File: sender.c
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "rsa_omp.h"

// RSA Keys (For demonstration purposes; in practice, use secure key generation and storage)
const uint64_t PUBLIC_KEY_E = 65537;
const uint64_t PUBLIC_KEY_N = 3233; // Example small modulus (replace with a large one)
// const uint64_t PRIVATE_KEY_D = 2753; // Not used in sender

// GUI Widgets
GtkWidget *button_select;
GtkWidget *button_send;
GtkWidget *text_view;
GtkWidget *entry_ip;
GtkWidget *entry_port;
GtkWidget *entry_file;

// Selected file path
char selected_file_path[1024] = {0};

// Function to update the text view
void update_text_view(const char *message) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, message, -1);
}

// Callback for file selection
void on_select_file(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Select File",
                                         GTK_WINDOW(data),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Open", GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        strncpy(selected_file_path, filename, sizeof(selected_file_path) - 1);
        gtk_entry_set_text(GTK_ENTRY(entry_file), filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

// Function to encrypt the file
int encrypt_file(const char *input_path, const char *output_path, uint64_t e, uint64_t n, char *encryption_info) {
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

    // Read file byte by byte, encrypt each byte
    int byte;
    uint64_t encrypted_byte;
    while ((byte = fgetc(fin)) != EOF) {
        encrypted_byte = modular_exponentiation_openmp((uint64_t)byte, e, n);
        fwrite(&encrypted_byte, sizeof(encrypted_byte), 1, fout);
    }

    fclose(fin);
    fclose(fout);

    // Prepare encryption info
    sprintf(encryption_info, "Encryption:\nPublic Key (e, n): (%llu, %llu)\n", e, n);

    return 0;
}

// Function to send the file via socket
int send_file_socket(const char *ip, int port, const char *file_path, char *encryption_info) {
    int sockfd;
    struct sockaddr_in server_addr;
    FILE *f = fopen(file_path, "rb");
    if (!f) {
        perror("fopen");
        return -1;
    }

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        fclose(f);
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, ip, &server_addr.sin_addr)<=0)  {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        fclose(f);
        return -1;
    }

    // Connect to receiver
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(sockfd);
        fclose(f);
        return -1;
    }

    // Send the file size first
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    long filesize_net = htonl(filesize);
    send(sockfd, &filesize_net, sizeof(filesize_net), 0);

    // Send the file data
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        send(sockfd, buffer, bytes_read, 0);
    }

    fclose(f);
    close(sockfd);

    // Update encryption info
    strcat(encryption_info, "File encrypted and sent successfully.\n");

    return 0;
}

// Callback for send button
void on_send_file(GtkWidget *widget, gpointer data) {
    const char *ip = gtk_entry_get_text(GTK_ENTRY(entry_ip));
    const char *port_str = gtk_entry_get_text(GTK_ENTRY(entry_port));
    int port = atoi(port_str);
    const char *file_path = gtk_entry_get_text(GTK_ENTRY(entry_file));

    if (strlen(file_path) == 0) {
        update_text_view("No file selected.\n");
        return;
    }

    // Encrypt the file
    char encrypted_file_path[1024];
    snprintf(encrypted_file_path, sizeof(encrypted_file_path), "%s.enc", file_path);

    char encryption_info[512] = {0};
    if (encrypt_file(file_path, encrypted_file_path, PUBLIC_KEY_E, PUBLIC_KEY_N, encryption_info) != 0) {
        update_text_view("Encryption failed.\n");
        return;
    }

    update_text_view(encryption_info);

    // Send the encrypted file
    if (send_file_socket(ip, port, encrypted_file_path, encryption_info) != 0) {
        update_text_view("Failed to send the file.\n");
        return;
    }

    update_text_view(encryption_info);
}

// Main function
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Create main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "RSA File Sender");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    // Create vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // IP Entry
    GtkWidget *hbox_ip = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label_ip = gtk_label_new("Receiver IP:");
    entry_ip = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_ip), "127.0.0.1"); // Default IP
    gtk_box_pack_start(GTK_BOX(hbox_ip), label_ip, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_ip), entry_ip, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_ip, FALSE, FALSE, 5);

    // Port Entry
    GtkWidget *hbox_port = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label_port = gtk_label_new("Receiver Port:");
    entry_port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_port), "5001"); // Default port
    gtk_box_pack_start(GTK_BOX(hbox_port), label_port, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_port), entry_port, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_port, FALSE, FALSE, 5);

    // File Selection
    GtkWidget *hbox_file = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label_file = gtk_label_new("Selected File:");
    entry_file = gtk_entry_new();
    gtk_entry_set_tabs(GTK_ENTRY(entry_file), FALSE); // Valid in GTK+ 3
    button_select = gtk_button_new_with_label("Select File");
    g_signal_connect(button_select, "clicked", G_CALLBACK(on_select_file), window);
    gtk_box_pack_start(GTK_BOX(hbox_file), label_file, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_file), entry_file, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_file), button_select, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_file, FALSE, FALSE, 5);

    // Send Button
    button_send = gtk_button_new_with_label("Encrypt and Send File");
    g_signal_connect(button_send, "clicked", G_CALLBACK(on_send_file), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button_send, FALSE, FALSE, 5);

    // Text View for Encryption Info
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
