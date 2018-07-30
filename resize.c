#include <cs50.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "bmp.h"

int main(int argc, char *argv[])
{
	// check that there are 3 cmd line inputs
	if (argc != 4) { return 1; }

	// read scaling factor
	char *endptr;
	errno = 0;
	float f = strtof(argv[1], &endptr);

	// check that scaling factor is a valid floating-point value
	if ((errno != 0) || ((f == 0) && (endptr == argv[1]))) 
	{
		fprintf(stderr, "First command line input is not a valid float.\n");
		return 1;
	}

	// check that scaling factor is in (0.0, 100.0]
	if ((f <= 0) || (f > 100))
	{
		fprintf(stderr, "Scaling factor needs to be (0.0, 100.0].\n");
		return 1;
	}

	// read name of input bitmap file
	char *infile = argv[2];

	// open input file
	FILE *inptr = fopen(infile, "r");
    if (inptr == NULL)
    {
        fprintf(stderr, "Could not open %s.\n", infile);
        return 1;
    }

	// read name of output bitmap file
	char *outfile = argv[3];	

	// open output file
    FILE *outptr = fopen(outfile, "w");
    if (outptr == NULL)
    {
        fclose(inptr);
        fprintf(stderr, "Could not create %s.\n", outfile);
        return 1;
    }

    // read infile's BITMAPFILEHEADER
    BITMAPFILEHEADER bf;
    fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);

    // read infile's BITMAPINFOHEADER
    BITMAPINFOHEADER bi;
    fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);

	// ensure infile is (likely) a 24-bit uncompressed BMP 4.0
    if (bf.bfType != 0x4d42 || bf.bfOffBits != 54 || bi.biSize != 40 ||
        bi.biBitCount != 24 || bi.biCompression != 0)
    {
        fclose(outptr);
        fclose(inptr);
        fprintf(stderr, "Unsupported file format.\n");
        return 1;
    }

    // copy to outfile's BITMAPFILEHEADER
    fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, outptr);
	BITMAPFILEHEADER bfo;
	fread(&bfo, sizeof(BITMAPFILEHEADER), 1, outptr);

    // copy to outfile's BITMAPINFOHEADER
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, outptr);
    BITMAPINFOHEADER bio;
    fread(&bio, sizeof(BITMAPINFOHEADER), 1, outptr);

    // update the width and height (does not include padding)
    bio.biWidth = round(bi.biWidth * f);
    bio.biHeight = round(bi.biHeight * f);

	// calculate the amount of padding needed in the new image
    int byteSize = 3;
    int requiredMultiple = 4;
    int padding = (requiredMultiple - (bio.biWidth * byteSize) % requiredMultiple) % requiredMultiple;

    // calculate the amount of padding in the original image
    int oldPadding = (requiredMultiple - (bi.biWidth * byteSize) % requiredMultiple) % requiredMultiple;

    // update biSizeImage
    bio.biSizeImage = ((sizeof(RGBTRIPLE) * bio.biWidth) + padding) * abs(bio.biHeight);

    // update bfSize
    bfo.bfSize = bio.biSizeImage + sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER);
    printf("Old width was: %i\nNew width is: %i\n", bi.biWidth, bio.biWidth);
    printf("Old height was: %i\nNew height is: %i\n", abs(bi.biHeight), abs(bio.biHeight));

    fseek(outptr, sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER), SEEK_SET);

    // go through all the rows in the new image
    for (int i = 0; i < abs(bio.biHeight); i++)
    {
    	// for each new row, decide which row from the original image should be copied over
    	// we use a floor function to ensure that the selected row never exceeds the height of the original image
    	double selectedRow = floor((double) i / (double) f);
    	printf("Selected row is: %f\n", selectedRow);

    	// go through all the columns in the given row in the new image
    	for (int j = 0; j < bio.biWidth; j++)
    	{
    		// for each new column, decide which column from the original image should be copied
    		double selectedColumn = floor((double) j / (double) f);
    		printf("Selected column is: %f\n", selectedColumn);

    		// navigate to the selected pixel in the original image
    		long location = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) + (selectedRow * (oldPadding + bi.biWidth * sizeof(RGBTRIPLE))) + (selectedColumn * sizeof(RGBTRIPLE));
    	
    		//printf("Current location is: %i\n", location);

    		RGBTRIPLE triple;
    		fseek(inptr, location, SEEK_SET);
    		fread(&triple, sizeof(RGBTRIPLE), 1, inptr);
    		fwrite(&triple, sizeof(RGBTRIPLE), 1, outptr);
    	}

    	// add padding to the end of each row in the new image
        for (int k = 0; k < padding; k++)
        {
            fputc(0x00, outptr);
        }
    }

	// goodbye!
	return 0;
}
