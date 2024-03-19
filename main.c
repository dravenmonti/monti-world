// Extension of Monti Noise
// Authored by Draven Monti
// Uses https://github.com/nothings/stb for image rendering

#include <gtk/gtk.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Convert two coordinates into a hash using xorshift
uint64_t hash(uint64_t x, uint64_t y) {
  x ^= x << 13 + y << 13;
  y ^= x >> 7 + y >> 7;
  x ^= x << 17 + y << 17;
  y ^= x << 13 + y << 13;
  x ^= x >> 7 + y >> 7;
  y ^= x << 17 + y << 17;
  return x;
}

// Convert coordinate hash to grid, and interpolate grid with zoomed + rotated
// hash (stronger at edges)
double monti_height(double x, double y, int depth, int seed) {
  int x_i = floor(x), y_i = floor(y);
  uint64_t h =
      hash(x_i * 7919 + y_i * 7907 + seed, y_i * 6277 - x_i * 6053 + seed);

  double out = (h % 6700417) / 6700417.0;

  if (depth == 0)
    return 0.5;

  double dist = (0.5 - fabs(x - x_i - 0.5)) * (0.5 - fabs(y - y_i - 0.5)) * 4;
  dist *= 0.9;

  out = out * dist + monti_height(x * 1.5 + y * 0.2, y * 1.5 - x * 0.2,
                                  depth - 1, seed + 1) *
                         (1.0 - dist);

  return out;
}

// Rivers going down a height gradient
double monti_river(double x, double y, int depth, int seed) {
  double p_0_0 = monti_height(x, y, depth, seed),
         p_0_1 = monti_height(x + 0.01, y + 0.01, depth, seed),
         p_1_0 = monti_height(x, y + .5, depth, seed),
         p_1_1 = monti_height(x + 0.01, y + 0.01, depth, seed),
         river = monti_height(x / 1.5, y / 1.5, depth / 2, seed + 69);

  double grad_x = (p_0_0 + p_0_1) - (p_1_0 + p_1_1);
  double grad_y = (p_0_0 + p_1_0) - (p_0_1 + p_1_1);

  double grad_d = sqrt(grad_x * grad_x + grad_y * grad_y) * 0.5 - 0.1;

  double smt = fabs(river - 0.5);

  smt *= 3;

  return (smt < grad_d) ? (smt / grad_d) : 1;
}

// Convert integer to string, useful for parsing command line arguments
int to_int(char *buf) {
  int i = 0, x = 0;
  for (; buf[x] >= '0' && buf[x] <= '9'; x++) {
    i = i * 10 + buf[x] - '0';
  }
  return i;
}

// Convert double to string
double to_double(char *buf) {
  int i = 0, x = 0;
  for (; buf[x] >= '0' && buf[x] <= '9'; x++) {
    i = i * 10 + buf[x] - '0';
  }
  if (buf[x] != '.') {
    return i;
  }
  x++;
  double place = 1;
  for (; buf[x] >= '0' && buf[x] <= '9'; x++) {
    place *= 0.1;
    i = i * 10 + buf[x] - '0';
  }

  return i * place;
}

void output_image(int width, int height, int scale, int seed, int mode,
                  char *output) {
  unsigned char *data = malloc(width * height * 4);

  // This will probably need to be refactored soon.
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      int c = i * width * 4 + j * 4;

      double x = i / 3000.0 * scale, y = j / 3000.0 * scale;

      double val = monti_height(x, y, 18, seed);
      int is_river = 0;
      if (val > 0.58) {
        double river = monti_river(x, y, 18, seed);
        val = val - 0.2 + 0.2 * river;
        is_river = river < 0.85;
      }
      if (val > 0.99)
        val = 0.99;

      data[c + 3] = 255;

      if (mode == 1) {
        data[c] = val * 255;

        // Biome
        data[c + 1] |= (!is_river && val > 0.6) << 7;
        data[c + 1] |= (is_river && val > 0.6) << 6;
        data[c + 1] |= (!is_river && val > 0.6 && val < 0.64) << 5;

        continue;
      }

      data[c] = (val > 0.6 && val < 0.64 && !is_river) ? 255 : val;
      data[c + 1] =
          ((val > 0.6 && !is_river) ? 255 : 0) * 0.7 + val * 255 * 0.3;
      data[c + 2] =
          ((val > 0.6 && !is_river) ? 0 : 255) * 0.3 + val * 255 * 0.7;

      double dec = (((int)(val * 256.0)) % 2) ? 1.0 : 0.9;

      data[c] *= dec;
      data[c + 1] *= dec;
      data[c + 2] *= dec;
    }
    printf("rendered %i / %i\n", i, height);
  }

  stbi_write_png(output, width, height, 4, data, width * 4);

  free(data);
}

GtkEntry *field_w, *field_h, *field_sc, *field_s, *field_mo, *field_o, *app;

static void on_button_clicked(GtkWidget *window) {
  int width = to_int(gtk_editable_get_text(GTK_EDITABLE(field_w))),
      height = to_int(gtk_editable_get_text(GTK_EDITABLE(field_h))),
      seed = to_int(gtk_editable_get_text(GTK_EDITABLE(field_s))),
      mode = to_int(gtk_editable_get_text(GTK_EDITABLE(field_mo)));
  double scale = to_double(gtk_editable_get_text(GTK_EDITABLE(field_sc)));
  output_image(width, height, scale, seed, mode,
               gtk_editable_get_text(GTK_EDITABLE(field_o)));

  GtkWidget *dialog = gtk_message_dialog_new(
      NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
      "Image created in this directory.");
  gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                           "Image created in this directory.");
  gtk_window_set_application(GTK_WINDOW(dialog), app);
  g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
  gtk_window_present (GTK_WINDOW (dialog));
}

// UI
static void on_activate(GtkApplication *app) {
  GtkWidget *window = gtk_application_window_new(app);

  GtkWidget *button = gtk_button_new_with_label("Create file");
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(on_button_clicked),
                           window);

  // TODO: cleanup
  field_w = gtk_entry_new();
  field_h = gtk_entry_new();
  field_sc = gtk_entry_new();
  field_s = gtk_entry_new();
  field_mo = gtk_entry_new();
  field_o = gtk_entry_new();

  GtkWidget *label_w = gtk_label_new("Width");
  GtkWidget *label_h = gtk_label_new("Height");
  GtkWidget *label_sc = gtk_label_new("Scale");
  GtkWidget *label_s = gtk_label_new("Seed");
  GtkWidget *label_mo = gtk_label_new("Mode (0 or 1)");
  GtkWidget *label_o = gtk_label_new("Output name");

  GtkWidget *grid = gtk_grid_new();

  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);

  gtk_grid_attach(GTK_GRID(grid), label_w, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), label_h, 1, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), label_sc, 2, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), label_s, 3, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), label_mo, 4, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), label_o, 5, 0, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), field_w, 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), field_h, 1, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), field_sc, 2, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), field_s, 3, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), field_mo, 4, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), field_o, 5, 1, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), button, 0, 2, 6, 1);

  gtk_window_set_child(GTK_WINDOW(window), grid);
  gtk_window_present(GTK_WINDOW(window));
}

// Entry point
int main(int argc, char *argv[]) {
  app = gtk_application_new("dev.dravenmonti.MontiWorld",
                                            G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  return g_application_run(G_APPLICATION(app), argc, argv);
}
