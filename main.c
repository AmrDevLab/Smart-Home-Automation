/*
 * Smart_Home.c
 *
 * Created: 10/12/2024 11:47:48 AM
 * Author : Eng. Amr Khaled El_Mansy
 */ 

#define F_CPU 16000000L
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>  

#define F_CPU 16000000L
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

// Function to send a command to the LCD
void LCD_Command(unsigned char cmd)
{
    PORTD = (PORTD & 0x0F) | (cmd & 0xF0);  // Send higher nibble
    PORTD &= ~(1 << 2);  // RS = 0 for command mode
    PORTD |= (1 << 3);   // Enable pulse
    _delay_us(1);
    PORTD &= ~(1 << 3);  // Disable pulse
    _delay_ms(2);
    
    PORTD = (PORTD & 0x0F) | (cmd << 4);  // Send lower nibble
    PORTD |= (1 << 3);  
    _delay_us(1);
    PORTD &= ~(1 << 3);  
    _delay_ms(2);
}

// Function to send a character to the LCD
void LCD_Char(unsigned char data)
{
    PORTD = (PORTD & 0x0F) | (data & 0xF0);  // Send higher nibble
    PORTD |= (1 << 2);  // RS = 1 for data mode
    PORTD |= (1 << 3);  // Enable pulse
    _delay_us(1);
    PORTD &= ~(1 << 3);  // Disable pulse
    _delay_ms(2);
    
    PORTD = (PORTD & 0x0F) | (data << 4);  // Send lower nibble
    PORTD |= (1 << 3);  
    _delay_us(1);
    PORTD &= ~(1 << 3);  
    _delay_ms(2);
}

// LCD initialization function
void LCD_Init(void)
{
    DDRD = 0xFF;  // Set PORTD as output for LCD
    _delay_ms(2);  

    LCD_Command(0x02);  // Initialize LCD in 4-bit mode
    LCD_Command(0x28);  // Configure 2-line, 5x7 matrix
    LCD_Command(0x0C);  // Display ON, Cursor OFF
    LCD_Command(0x06);  // Auto-increment cursor
    LCD_Command(0x01);  // Clear display
    _delay_ms(20);
}

// Function to display a string on the LCD
void LCD_String(char *str)
{
    while (*str)
    {
        LCD_Char(*str++);
    }
}

// Function to display a message on the LCD after clearing it
void displayMessage(char* message) {
    LCD_Command(0x01);  // Clear display
    LCD_String(message);  
}

// ADC initialization function
void ADC_Init(void)
{
    ADMUX = (1 << REFS0);  // Select AVcc as reference
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Enable ADC with prescaler 128
}

// Function to read ADC value from the selected channel
unsigned int ADC_Read(unsigned char channel)
{
    channel &= 0x07;  // Ensure channel is between 0-7
    ADMUX = (ADMUX & 0xF8) | channel;  // Select ADC channel
    ADCSRA |= (1 << ADSC);  // Start conversion
    while (ADCSRA & (1 << ADSC));  // Wait for conversion to finish
    return ADC;
}

int main(void)
{
    // Configure pin directions for LEDs and sensors
    DDRB |= (1 << 1);  // Set PB1 as output (fire alarm)
    PORTB |= (1 << 0);  // Enable pull-up on PB0 (fire sensor)
    DDRB &= ~(1 << 0);  // Set PB0 as input (fire sensor)
    
    DDRB |= (1 << 3);  // Set PB3 as output (lamp)
    PORTB |= (1 << 2);  // Enable pull-up on PB2 (light sensor)
    DDRB &= ~(1 << 2);  // Set PB2 as input (light sensor)
    
    DDRB |= (1 << 4);  // Set PB4 as output (fan control)
    
    ADC_Init();  // Initialize ADC for temperature sensing
    LCD_Init();  // Initialize LCD for displaying information
    
    char buffer[16];  // Buffer for LCD display messages
    uint8_t LDRValue = 0, fireVal = 0;  // Variables to store sensor states
	
    while (1)
    {
        // Display fire alarm status on LCD
        sprintf(buffer, "Fire alarm:%d ", fireVal);  
        displayMessage(buffer);  
        
        // Display lamp status on LCD
        sprintf(buffer, "LAMP is: %d", LDRValue);  
        displayMessage(buffer);  
            
        // Check fire sensor and control fire alarm
        if ((PINB & (1 << 0)) == 1)  // If fire sensor is triggered
        {
            PORTB |= (1 << 1);  // Turn on fire alarm
            fireVal = 1;
        }
        else  // If no fire detected
        {
            PORTB &= ~(1 << 1);  // Turn off fire alarm
            fireVal = 0;
        }

        // Check light sensor and control lamp
        if ((PINB & (1 << 2)) == 0)  // If light sensor is not triggered
        {
            PORTB &= ~(1 << 3);  // Turn off lamp
            LDRValue = 0;
        }
        else  // If light sensor is triggered
        {
            PORTB |= (1 << 3);  // Turn on lamp
            LDRValue = 1;
        }

        // Read temperature sensor via ADC and calculate temperature
        unsigned int adc_value = ADC_Read(0);  
        float temperature = (adc_value / 1024.0) * 500.0;  // Convert ADC value to temperature
        int int_temp = (int)temperature;  

        // Display temperature on LCD
        LCD_Command(0xC0);  // Move cursor to second line
        sprintf(buffer, "Temp: %d C", int_temp);  
        LCD_String(buffer);  

        // Control fan based on temperature
        if (int_temp > 30)  // If temperature exceeds 30°C
        {
            PORTB |= (1 << 4);  // Turn on fan
        }
        else  
        {
            PORTB &= ~(1 << 4);  // Turn off fan
        }
		
        _delay_ms(400);  // Wait before next cycle
    }

    return 0;
}
