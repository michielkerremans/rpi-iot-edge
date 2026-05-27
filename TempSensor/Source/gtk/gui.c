#include <gtk/gtk.h>
#include "gui.h"
#include "../gpio/gpio.h"
#include "../mqtt/mqtt.h"

// ====== Data Structures and State ======

// Output control structure for GPIO outputs
typedef struct
{
  int pin;
  int *state;
  GtkSwitch *sw;
  guint *timer_id;
} GpioControl;

// Temperature label widget
static GtkWidget *temp_label = NULL;

// GPIO input state and LED widgets
static int gpio26_state = 0, gpio27_state = 0;
static GtkWidget *gpio26_led = NULL, *gpio27_led = NULL;

// GPIO output state and timer IDs
static int gpio17_state = 0, gpio19_state = 0;
static guint timer17_id = 0, timer19_id = 0;

// Output control structs for GPIO 17 and 19
static GpioControl ctrl17 = {17, &gpio17_state, NULL, &timer17_id};
static GpioControl ctrl19 = {19, &gpio19_state, NULL, &timer19_id};

// ====== Callback Functions ======

// Draws an LED for GPIO input state
static gboolean draw_led(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  int state = *(int *)user_data;
  cairo_arc(cr, 10, 10, 8, 0, 2 * G_PI);
  cairo_set_source_rgb(cr, state ? 0 : 1, state ? 1 : 0, 0);
  cairo_fill(cr);
  return FALSE;
}

// [GTK] Cairo is a 2D graphics library used by GTK for custom drawing (e.g., shapes, colors) on widgets.

// Updates the temperature label from MQTT
static gboolean update_temp_label(gpointer user_data)
{
  const char *payload = mqtt_get_last_payload();
  char buf[128];
  if (payload[0])
    snprintf(buf, sizeof(buf), "Temperature:  <span size=\"x-large\"><b>%s</b></span> <span size=\"x-large\">°C</span>", payload);
  else
    snprintf(buf, sizeof(buf), "Temperature:  <span size=\"x-large\"><b>--</b></span> <span size=\"x-large\">°C</span>");
  gtk_label_set_markup(GTK_LABEL(temp_label), buf); // Sets styled label text (markup in buf)
  return TRUE;
}

/* [GTK] GTK code is typically written using generic GtkWidget * pointers and type macros
 * (like GTK_LABEL(), GTK_SWITCH()) because C doesn’t support real inheritance or templates.
 * All widgets must be handled as GtkWidget * when adding them to containers or connecting signals.
 * The macros (e.g., GTK_LABEL()) cast the generic pointer to the specific widget type (e.g., GtkLabel)
 * so you can safely call widget-specific functions.
 */

// Reads GPIO input and redraws LEDs
static gboolean update_gpio_inputs(gpointer user_data)
{
  GPIO_Read(26, &gpio26_state);
  GPIO_Read(27, &gpio27_state);
  gtk_widget_queue_draw(gpio26_led); // Schedules redraw of the LED widget to reflect new state
  gtk_widget_queue_draw(gpio27_led); // [GTK] draw requests are queued and processed by GTK's main loop
  return TRUE;
}

// Sets GPIO output state and updates switch
static void set_output_state(GpioControl *ctrl, int state)
{
  *(ctrl->state) = state;
  GPIO_Write(ctrl->pin, state);
  gtk_switch_set_active(ctrl->sw, state);
  update_gpio_inputs(NULL); // Immediately read the inputs again (and update LEDs)
}

// Callback for manual output control via GtkSwitch
static gboolean on_switch(GtkSwitch *sw, gboolean state, gpointer user_data)
{
  set_output_state((GpioControl *)user_data, state);
  return FALSE;
}

// Callback for periodic output toggling (timer)
static gboolean toggle_gpio_cb(gpointer user_data)
{
  set_output_state((GpioControl *)user_data, !*(((GpioControl *)user_data)->state));
  return TRUE;
}

// Callback for timer interval changes via GtkSpinButton
static void on_interval_changed(GtkSpinButton *spin, gpointer user_data)
{
  GpioControl *ctrl = (GpioControl *)user_data;
  int val = gtk_spin_button_get_value_as_int(spin);
  if (*(ctrl->timer_id))
    g_source_remove(*(ctrl->timer_id)); // [GTK] Removes the active timer callback from the main loop
  if (val > 0)
    *(ctrl->timer_id) = g_timeout_add_seconds(val, toggle_gpio_cb, ctrl);
}

// Callback for the output toggle button
static void on_output_toggle(GtkButton *btn, gpointer user_data)
{
  GtkComboBox *combo = GTK_COMBO_BOX(user_data);
  int idx = gtk_combo_box_get_active(combo);
  if (idx == 0)
  {
    set_output_state(&ctrl17, !(*ctrl17.state));
  }
  else if (idx == 1)
  {
    set_output_state(&ctrl19, !(*ctrl19.state));
  }
}

// Cleanup handler for window close
static void on_window_destroy(GtkWidget *widget, gpointer user_data)
{
  mqtt_cleanup();
  GPIO_Cleanup();
  gtk_main_quit();
}

// ====== GUI Construction and Main Entry ======

void launch_gtk_gui(void)
{
  // --- Initialization ---
  int argc = 0;
  char **argv = NULL;
  gtk_init(&argc, &argv);
  GPIO_Init();
  GPIO_Mode(17, 1);
  GPIO_Mode(19, 1);
  GPIO_Mode(26, 0);
  GPIO_Mode(27, 0);

  // Read initial GPIO input states
  GPIO_Read(26, &gpio26_state);
  GPIO_Read(27, &gpio27_state);
  gpio19_state = gpio26_state;
  gpio17_state = gpio27_state;
  // Write initial output states to match inputs
  GPIO_Write(17, gpio17_state);
  GPIO_Write(19, gpio19_state);

  // --- Window and Layout ---
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "GPIO Toggle GUI");
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_container_add(GTK_CONTAINER(window), grid);

  // --- Top: Temperature ---
  temp_label = gtk_label_new("Temperature:  <span size=\"x-large\"><b>--</b></span> <span size=\"x-large\">°C</span>");
  gtk_label_set_xalign(GTK_LABEL(temp_label), 0.0);
  gtk_label_set_use_markup(GTK_LABEL(temp_label), TRUE);
  gtk_grid_attach(GTK_GRID(grid), temp_label, 0, 0, 6, 1);
  // Add a separator with padding below temperature
  GtkWidget *temp_sep_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *temp_sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start(GTK_BOX(temp_sep_box), temp_sep, TRUE, TRUE, 2); // 2px padding
  gtk_grid_attach(GTK_GRID(grid), temp_sep_box, 0, 1, 6, 1);

  // [GTK] gtk_box_pack_start() adds a widget to the start of a GtkBox (“packing” means inserting in order)

  // --- Row 1: GPIO 27 (IN) ---
  GtkWidget *label27 = gtk_label_new("GPIO 27 (IN)");
  gpio27_led = gtk_drawing_area_new();
  gtk_widget_set_size_request(gpio27_led, 20, 20);
  gtk_widget_set_valign(gpio27_led, GTK_ALIGN_CENTER);
  gtk_grid_attach(GTK_GRID(grid), label27, 0, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), gpio27_led, 1, 2, 1, 1);
  g_signal_connect(gpio27_led, "draw", G_CALLBACK(draw_led), &gpio27_state);

  // [GTK] g_signal_connect() links a widget’s signal (like "draw" or "state-set", as defined by GTK) to a callback function

  // --- Row 1: GPIO 17 (OUT) ---
  GtkWidget *label17 = gtk_label_new("GPIO 17 (OUT)");
  GtkWidget *switch17 = gtk_switch_new();
  GtkWidget *spin17 = gtk_spin_button_new_with_range(0, 60, 1);
  GtkWidget *label17s = gtk_label_new("seconds");
  gtk_grid_attach(GTK_GRID(grid), label17, 3, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), switch17, 4, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), spin17, 5, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), label17s, 6, 2, 1, 1);
  gtk_switch_set_active(GTK_SWITCH(switch17), gpio17_state); // Sync initial switch state with GPIO state

  // --- Row 2: GPIO 26 (IN) ---
  GtkWidget *label26 = gtk_label_new("GPIO 26 (IN)");
  gpio26_led = gtk_drawing_area_new();
  gtk_widget_set_size_request(gpio26_led, 20, 20);
  gtk_widget_set_valign(gpio26_led, GTK_ALIGN_CENTER);
  gtk_grid_attach(GTK_GRID(grid), label26, 0, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), gpio26_led, 1, 3, 1, 1);
  g_signal_connect(gpio26_led, "draw", G_CALLBACK(draw_led), &gpio26_state);

  // --- Row 2: GPIO 19 (OUT) ---
  GtkWidget *label19 = gtk_label_new("GPIO 19 (OUT)");
  GtkWidget *switch19 = gtk_switch_new();
  GtkWidget *spin19 = gtk_spin_button_new_with_range(0, 60, 1);
  GtkWidget *label19s = gtk_label_new("seconds");
  gtk_grid_attach(GTK_GRID(grid), label19, 3, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), switch19, 4, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), spin19, 5, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), label19s, 6, 3, 1, 1);
  gtk_switch_set_active(GTK_SWITCH(switch19), gpio19_state); // Sync initial switch state with GPIO state

  // Link switches to control structs
  ctrl17.sw = GTK_SWITCH(switch17);
  ctrl19.sw = GTK_SWITCH(switch19);

  // --- Signal Connections ---
  g_signal_connect(switch17, "state-set", G_CALLBACK(on_switch), &ctrl17);
  g_signal_connect(switch19, "state-set", G_CALLBACK(on_switch), &ctrl19);
  g_signal_connect(spin17, "value-changed", G_CALLBACK(on_interval_changed), &ctrl17);
  g_signal_connect(spin19, "value-changed", G_CALLBACK(on_interval_changed), &ctrl19);
  // g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

  // --- Divider and Output Toggle Section ---
  // Add a horizontal separator below everything
  GtkWidget *bottom_sep_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *bottom_sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start(GTK_BOX(bottom_sep_box), bottom_sep, TRUE, TRUE, 2);
  gtk_grid_attach(GTK_GRID(grid), bottom_sep_box, 0, 4, 7, 1);

  // Dropdown (combo box) for selecting output GPIO
  GtkWidget *output_combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(output_combo), "GPIO 17 (OUT)");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(output_combo), "GPIO 19 (OUT)");
  gtk_combo_box_set_active(GTK_COMBO_BOX(output_combo), 0);

  // Toggle button
  GtkWidget *toggle_btn = gtk_button_new_with_label("Toggle");

  // Attach combo and button to grid (row 5, columns 0-2)
  gtk_grid_attach(GTK_GRID(grid), output_combo, 0, 5, 2, 1);
  gtk_grid_attach(GTK_GRID(grid), toggle_btn, 2, 5, 1, 1);

  // Connect toggle button signal
  g_signal_connect(toggle_btn, "clicked", G_CALLBACK(on_output_toggle), output_combo);

  // --- Show and Start ---
  gtk_widget_show_all(window);                        // [GTK] Shows the window and all child widgets
  g_timeout_add_seconds(1, update_temp_label, NULL);  // [GTK] Calls update_temp_label every second
  g_timeout_add_seconds(1, update_gpio_inputs, NULL); // [GTK] Calls update_gpio_inputs every second
  gtk_main();
}