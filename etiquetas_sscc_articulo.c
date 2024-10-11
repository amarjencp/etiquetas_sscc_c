
/*
 * Programa: etiquetas_sscc.c
 * Versión: 1.2.0
 * Autor: Tony M. Jenkins
 * Descripción: Genera etiquetas logísticas con números SSCC desde un CSV,
 * y añade información del palet y articulo dentro de la caja.
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
#include <assert.h>
#include <zint.h>
#include <hpdf.h>

#define A6_WIDTH (148.0 * 2.83464567)  // 148 mm in points
#define A6_HEIGHT (105.0 * 2.83464567) // 105 mm in points
#define MAX_CSV_LINES 1000  // Limit the number of lines processed from the CSV

// Function to validate SSCC length and check digit (ignores first 2 AI digits)
int is_valid_sscc(const char *sscc) {

    if (strlen(sscc) != 20) {
        return 0; // Invalid length
    }
    for (int i = 0; i < 20; i++) {
        if (!isdigit(sscc[i])) {
            return 0; // Contains non-digit characters
        }
    }
    int sum = 0;
    int multiplier = 3;
    for (int i = 18; i >= 2; i--) {
        int digit = sscc[i] - '0';
        sum += digit * multiplier;
        multiplier = (multiplier == 3) ? 1 : 3;
    }
    int check_digit_calculated = (10 - (sum % 10)) % 10;
    int check_digit_provided = sscc[19] - '0';
    return check_digit_calculated == check_digit_provided;
}

void generate_barcode(const char *code, const char *filename) {
    assert(code != NULL);
    assert(filename != NULL);

    struct zint_symbol *my_symbol = ZBarcode_Create();
    assert(my_symbol != NULL); // Runtime check to ensure the barcode object was created.

    strcpy(my_symbol->outfile, filename);
    my_symbol->symbology = BARCODE_CODE128;
    my_symbol->output_options = BARCODE_QUIET_ZONES | BARCODE_BOX;
    my_symbol->show_hrt = 0;
    my_symbol->height = 40;
    my_symbol->scale = 1;

    int encode_status = ZBarcode_Encode_and_Print(my_symbol, (unsigned char *)code, 0, 0);
    assert(encode_status == 0);  // Check for successful barcode generation.

    ZBarcode_Delete(my_symbol);
}

void create_label(HPDF_Doc pdf, HPDF_Font font, const char *sscc, const char *palet, const char *articulo) {
    assert(pdf != NULL);
    assert(font != NULL);
    assert(sscc != NULL);

    char formatted_sscc[23];
    snprintf(formatted_sscc, sizeof(formatted_sscc), "(%.2s)%s", sscc, sscc + 2);

    char filename[64];
    snprintf(filename, sizeof(filename), "barcode_%s.png", sscc);
    generate_barcode(sscc, filename);

    HPDF_Page page = HPDF_AddPage(pdf);
    assert(page != NULL); // Check that a new PDF page was successfully created.

    HPDF_Page_SetWidth(page, A6_WIDTH);
    HPDF_Page_SetHeight(page, A6_HEIGHT);

    HPDF_Image barcode_image = HPDF_LoadPngImageFromFile(pdf, filename);
    assert(barcode_image != NULL); // Ensure the barcode image is loaded.

    HPDF_REAL barcode_width = A6_WIDTH * 0.8;
    HPDF_REAL barcode_height = A6_HEIGHT * 0.25;
    HPDF_REAL barcode_x = (A6_WIDTH - barcode_width) / 2;
    HPDF_REAL barcode_y = A6_HEIGHT - barcode_height - 30;

    HPDF_Page_DrawImage(page, barcode_image, barcode_x, barcode_y, barcode_width, barcode_height);

    HPDF_Page_SetFontAndSize(page, font, 18);
    HPDF_Page_BeginText(page);
    HPDF_REAL text_width = HPDF_Page_TextWidth(page, formatted_sscc);
    HPDF_Page_TextOut(page, (A6_WIDTH - text_width) / 2, barcode_y - 20, formatted_sscc);
    HPDF_Page_EndText(page);

    // Draw the box for PALET and ARTICULO
    HPDF_Page_SetLineWidth(page, 2.0);
    HPDF_Page_Rectangle(page, 20, 30, A6_WIDTH - 40, A6_HEIGHT * 0.40);
    HPDF_Page_Stroke(page);

    // Add PALET and ARTICULO separately inside the box
    HPDF_Page_SetFontAndSize(page, font, 12);

    // Draw the PALET text
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page, 30, 60, palet);
    HPDF_Page_EndText(page);

    // Draw the ARTICULO text
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page, 30, 40, articulo);
    HPDF_Page_EndText(page);

    remove(filename);  // Ensure the temporary barcode image is deleted.
}

void process_csv_and_generate_labels(const char *csv_filename, HPDF_Doc pdf, HPDF_Font font) {
    assert(csv_filename != NULL);
    assert(pdf != NULL);
    assert(font != NULL);

    FILE *csv_file = fopen(csv_filename, "r");
    if (!csv_file) {
        perror("No se pudo abrir el archivo CSV");
        return;
    }

    char line[256];
    int line_count = 0;

    while (fgets(line, sizeof(line), csv_file) && line_count < MAX_CSV_LINES) {
        line[strcspn(line, "\n")] = 0;

        // Split line into SSCC, PALET, and ARTICULO
        char *sscc = strtok(line, ",");
        char *palet = strtok(NULL, ",");
        char *articulo = strtok(NULL, ",");

        if (!sscc ) {
            fprintf(stderr, "Línea CSV inválida: %s\n", line);
            continue;
        }

        if (!is_valid_sscc(sscc)) {
            fprintf(stderr, "Invalid SSCC: %s\n", sscc);
            continue;
        }

        create_label(pdf, font, sscc, palet, articulo);
        line_count++;
    }

    fclose(csv_file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <archivo CSV con SSCC, PALET y ARTICULO>\n", argv[0]);
        return 1;
    }

    const char *csv_filename = argv[1];

    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    if (!pdf) {
        fprintf(stderr, "Error al crear el objeto PDF.\n");
        return 1;
    }

    HPDF_Font font = HPDF_GetFont(pdf, "Courier-Bold", NULL);
    assert(font != NULL);  // Check font loading success.

    process_csv_and_generate_labels(csv_filename, pdf, font);

    HPDF_SaveToFile(pdf, "sscc_labels.pdf");

    HPDF_Free(pdf);

    return 0;
}
