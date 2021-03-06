#ifndef AVERAGE_H
#define AVERAGE_H

#define AVERAGE_SIZE    ( 50 )

class Average
{
    private:
        double values[AVERAGE_SIZE] = { 0 };
        double average = 0;
        int curIndex = 0;
        
        void calculateAverage();
        
    public:
        double getAverage();
        void addValue( double value );
        
        Average();

};

#endif // TEMPERATURE_H
