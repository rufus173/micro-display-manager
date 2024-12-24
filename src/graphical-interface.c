#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <stdint.h>

static GtkApplication *app;
static GtkEntryBuffer *username_entry_buffer;
static GtkEntryBuffer *password_entry_buffer;
static int (*login_button_function)(char *,char *) = NULL;

static void login_button_callback(GtkWidget *widget, gpointer data);
static void activate(GtkApplication *app, gpointer data);
void show_message_popup(char *);
void register_login_button_function(int (*registered_function)(char *,char *));
int start_gui(int argc, char **argv){
	int result;

	app = gtk_application_new("com.github.rufus173.DisplayManager",0); //ubuntu lts makes me sad cause no G_APPLICATION_DEFAULT_FLAGS
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	g_application_run(G_APPLICATION(app),argc,argv);
	g_object_unref(app);
	return result;
}
static void activate(GtkApplication *app, gpointer data){
	//make the window
	GtkWidget *window;
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window),"login");

	//make some nice widgets for the user to interact with
	GtkWidget *box; //store our widgets
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	//stuff the box into the window
	gtk_window_set_child(GTK_WINDOW(window),box);

	//username and password entry feilds with corresponding labels
	GtkWidget *username_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
	GtkWidget *username_label = gtk_label_new("username");
	GtkWidget *username_entry = gtk_entry_new();
	gtk_box_append(GTK_BOX(username_box),username_label);
	gtk_box_append(GTK_BOX(username_box),username_entry);
	gtk_box_append(GTK_BOX(box),username_box);
	username_entry_buffer = gtk_entry_get_buffer(GTK_ENTRY(username_entry));

	GtkWidget *password_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
	GtkWidget *password_label = gtk_label_new("password");
	GtkWidget *password_entry = gtk_entry_new();
	gtk_box_append(GTK_BOX(password_box),password_label);
	gtk_box_append(GTK_BOX(password_box),password_entry);
	gtk_box_append(GTK_BOX(box),password_box);
	password_entry_buffer = gtk_entry_get_buffer(GTK_ENTRY(password_entry));
	//make the password invisible when typing
	gtk_entry_set_visibility(GTK_ENTRY(password_entry),FALSE);

	//login button
	GtkWidget *login_button;
	login_button = gtk_button_new_with_label("Login");
	gtk_box_append(GTK_BOX(box),login_button);
	g_signal_connect(login_button,"clicked",G_CALLBACK(login_button_callback),window);

	//and... present!
	gtk_window_present(GTK_WINDOW(window));
}
void register_login_button_function(int (*registered_function)(char *,char *)){
	login_button_function = registered_function;
}
static void login_button_callback(GtkWidget *widget, gpointer data){
	char *username = NULL;
	char *password = NULL;
	//ik strdup exists im lazy ok
	size_t username_size = gtk_entry_buffer_get_length(username_entry_buffer) +1;
	size_t password_size = gtk_entry_buffer_get_length(password_entry_buffer) +1;
	username = calloc(0,username_size);
	password = calloc(0,password_size);
	snprintf(username,username_size,"%s",gtk_entry_buffer_get_text(username_entry_buffer));
	snprintf(password,password_size,"%s",gtk_entry_buffer_get_text(password_entry_buffer));

	//debug
	//show_message_popup("this is a message popup");

	//call the registered callback function
	int result = -1;
	if (login_button_function != NULL) result = login_button_function(username,password);
	printf("login func returned %d\n",result);

	free(username);
	free(password);
	//stop the ui if login was a success
	if (result == 0) gtk_window_destroy(GTK_WINDOW(data));//g_application_quit(G_APPLICATION(app));
}
void show_message_popup(char *message){
	GtkWidget *popup_window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(popup_window),"message");

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	gtk_window_set_child(GTK_WINDOW(popup_window),box);

	GtkWidget *message_label = gtk_label_new(message);
	gtk_box_append(GTK_BOX(box),message_label);
	
	GtkWidget *close_button = gtk_button_new_with_label("close");
	gtk_box_append(GTK_BOX(box),close_button);
	g_signal_connect_swapped(close_button,"clicked",G_CALLBACK(gtk_window_destroy),popup_window);

	gtk_window_present(GTK_WINDOW(popup_window));
}
