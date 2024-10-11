
/*
 * Programa: etiquetas_sscc.c
 * Versión: 1.0.0
 * Autor: Tony M. Jenkins
 * Descripción: Genera etiquetas logísticas con numeros SSCC desde un CSV.
 *
 * Dependencias:
 * - libzint version 2.13.0
 * - libhpdf version 2.4.4
 * - libpng version 1.6.43
 * - zlib version 1.3.1
 */

#include <stdio.h>
#include <stdlib.h> 
#include <ctype.h>
#include <string.h>
#include <zint.h>
#include <hpdf.h>

#define A6_WIDTH (148.0 * 2.83464567)  // 148 mm in points
#define A6_HEIGHT (105.0 * 2.83464567) // 105 mm in points

// Function to validate SSCC length and check digit (ignores first 2 AI digits)
int is_valid_sscc(const char *sscc) {
    // Check if the SSCC length is exactly 20 digits
    if (strlen(sscc) != 20) {
        return 0; // Invalid length
    }

    // Check if the SSCC contains only digits
    for (int i = 0; i < 20; i++) {
        if (!isdigit(sscc[i])) {
            return 0; // Contains non-digit characters
        }
    }

    // Calculate the check digit using the last 18 digits (ignore the first 2 digits)
    int sum = 0;
    int multiplier = 3; // Start with 3 for the first digit from the right
    for (int i = 18; i >= 2; i--) {  // Process the last 18 digits (ignoring the first 2)
        int digit = sscc[i] - '0';  // Convert character to integer
        sum += digit * multiplier;

        // Alternate the multiplier between 3 and 1
        multiplier = (multiplier == 3) ? 1 : 3;
    }

    // Calculate the check digit
    int check_digit_calculated = (10 - (sum % 10)) % 10;
    int check_digit_provided = sscc[19] - '0';  // The last digit in SSCC

    // Check if the provided check digit matches the calculated check digit
    if (check_digit_calculated == check_digit_provided) {
        return 1;
    } else {
        return 0;
    }
}

void generate_barcode(const char *code, const char *filename) {
    /// Function para generar código de barras y grabarlo en PNG
    struct zint_symbol *my_symbol = ZBarcode_Create();
    strcpy(my_symbol->outfile, filename);
    my_symbol->symbology = BARCODE_CODE128;
    my_symbol->output_options = BARCODE_QUIET_ZONES | BARCODE_BOX;
    my_symbol->show_hrt = 0;
    my_symbol->height = 40;
    my_symbol->scale = 1;
    ZBarcode_Encode_and_Print(my_symbol, (unsigned char *)code, 0, 0);
    ZBarcode_Delete(my_symbol);
}
void create_label(HPDF_Doc pdf, HPDF_Font font, const char *sscc) {
    // Format SSCC as (12)345678901234567890
    char formatted_sscc[23];
    snprintf(formatted_sscc, sizeof(formatted_sscc), "(%.2s)%s", sscc, sscc + 2);

    // Generate barcode image
    char filename[64];
    snprintf(filename, sizeof(filename), "barcode_%s.png", sscc);
    generate_barcode(sscc, filename);

    // Create a new page for the label
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetWidth(page, A6_WIDTH);
    HPDF_Page_SetHeight(page, A6_HEIGHT);

    // Load barcode image into the PDF
    HPDF_Image barcode_image = HPDF_LoadPngImageFromFile(pdf, filename);

    // Calculate positions
    HPDF_REAL barcode_width = A6_WIDTH * 0.8;  // Width of the barcode (80% of the page width)
    HPDF_REAL barcode_height = A6_HEIGHT * 0.25; // Height of the barcode (25% of the page height)
    HPDF_REAL barcode_x = (A6_WIDTH - barcode_width) / 2; // Center the barcode horizontally
    HPDF_REAL barcode_y = A6_HEIGHT - barcode_height - 30; // Position the barcode in the top half

    // Draw the barcode image in the top half
    HPDF_Page_DrawImage(page, barcode_image, barcode_x, barcode_y, barcode_width, barcode_height);

    // Draw the HRI text below the barcode
    HPDF_Page_SetFontAndSize(page, font, 18);
    HPDF_Page_BeginText(page);
    HPDF_REAL text_width = HPDF_Page_TextWidth(page, formatted_sscc);
    HPDF_Page_TextOut(page, (A6_WIDTH - text_width) / 2, barcode_y - 20, formatted_sscc);
    HPDF_Page_EndText(page);

    // Draw the box for manual writing in the bottom half
    HPDF_Page_SetLineWidth(page, 2.0);
    HPDF_Page_Rectangle(page, 20, 30, A6_WIDTH - 40, A6_HEIGHT * 0.40);
    HPDF_Page_Stroke(page);

    // Clean up the temporary barcode file
    remove(filename);
}
void create_label_old(HPDF_Doc pdf, HPDF_Font font, const char *sscc) {
    /// Función para generar etiqueta de SSCC en PDF
    char filename[64];
    snprintf(filename, sizeof(filename), "barcode_%s.png", sscc);

    generate_barcode(sscc, filename);

    // Crear nueva página para cada etiqueta
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetWidth(page, A6_WIDTH);
    HPDF_Page_SetHeight(page, A6_HEIGHT);

    // Cargar la imagen del código de barras en el PDF
    HPDF_Image image = HPDF_LoadPngImageFromFile(pdf, filename);

    // Pintar el código de barras en la página
    HPDF_Page_DrawImage(page, image, 20, 40, A6_WIDTH - 40, A6_HEIGHT - 80);

    // Añadir el texto SSCC debajo del código
    HPDF_Page_SetFontAndSize(page, font, 8);
    HPDF_Page_BeginText(page);

    // Create the formatted string in C
    char formatted_sscc[64];  // Ensure this is large enough for the formatted SSCC
    snprintf(formatted_sscc, sizeof(formatted_sscc), "(%.2s)%s", sscc, sscc + 2);

    // Calculate the width of the formatted string
    HPDF_REAL text_width = HPDF_Page_TextWidth(page, formatted_sscc);

    // Draw the formatted SSCC text in the center
    HPDF_Page_TextOut(page, (A6_WIDTH - text_width) / 2, 20, formatted_sscc);


    HPDF_Page_EndText(page);

    // Eliminar el archivo temporal del código de barras
    remove(filename);
}

void process_csv_and_generate_labels(const char *csv_filename, HPDF_Doc pdf, HPDF_Font font) {
    /// Función para procesar el archivo CSV y generar etiquetas
    FILE *csv_file = fopen(csv_filename, "r");
    if (!csv_file) {
        perror("No se pudo abrir el archivo CSV");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), csv_file)) {
        // Remove the newline character
        line[strcspn(line, "\n")] = 0;

        // Check if the SSCC is valid
        if (!is_valid_sscc(line)) {
            fprintf(stderr, "Invalid SSCC: %s\n", line);
            continue;  // Skip invalid SSCCs
        }

        // Create label for each valid SSCC
        create_label(pdf, font, line);
    }

    fclose(csv_file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <archivo CSV con SSCC>\n", argv[0]);
        return 1;
    }

    const char *csv_filename = argv[1];

    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    if (!pdf) {
        fprintf(stderr, "Error al crear el objeto PDF.\n");
        return 1;
    }

    HPDF_Font font = HPDF_GetFont(pdf, "Courier-Bold", NULL);

    // Procesar el archivo CSV y generar etiquetas en el PDF
    process_csv_and_generate_labels(csv_filename, pdf, font);

    // Guardar el archivo PDF
    HPDF_SaveToFile(pdf, "sscc_labels.pdf");

    // Limpiar el objeto PDF
    HPDF_Free(pdf);

    return 0;
}
