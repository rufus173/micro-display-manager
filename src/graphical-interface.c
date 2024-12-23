#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
static void activate(GtkApplication *app, gpointer data);
int start_gui(int argc, char **argv){
	GtkApplication *app;
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

	//login button
	GtkWidget *login_button;
	login_button = gtk_button_new_with_label("Login");
	gtk_box_append(GTK_BOX(box),login_button);

	//and... present!
	gtk_window_present(GTK_WINDOW(window));
}
