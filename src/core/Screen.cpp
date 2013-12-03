/*
    cheali-charger - open source firmware for a variety of LiPo chargers
    Copyright (C) 2013  Pawel Stawicki. All right reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <avr/pgmspace.h>
#include "Screen.h"
#include "LcdPrint.h"
#include "ProgramData.h"
#include "TheveninMethod.h"
#include "Settings.h"
#include "Hardware.h"
#include "Program.h"


namespace Screen{

    uint32_t startTime_totalTime_;
    uint32_t totalBalanceTime_;
    uint32_t totalChargDischargeTime_;
    Blink blink;
    bool blinkIcon;
#ifdef  KNIGHTRIDEREFFECT   
    int8_t knightRiderCounter = 0;
    bool knightRiderDir;
#endif
    uint8_t toggleTextCounter = 0;
    uint8_t toggleTextCycleCounter_ = 1;
    uint8_t procent_ = 0;
    uint8_t procent;
    uint16_t etaSec = 0;
    uint16_t timeSecOldETACalc;
    uint16_t etaSecLarge = 0;
    
    
    //TODO_NJ for cyclehistory
    uint16_t cyclesHistoryChCapacity[10] = {0,0,0,0,0,0,0,0,0,0};
    uint16_t cyclesHistoryDcCapacity[10] = {0,0,0,0,0,0,0,0,0,0};
    uint16_t cyclesHistoryTime[10]       = {0,0,0,0,0,0,0,0,0,0};
    char     cyclesHistoryMode[10]      = {'-','-','-','-','-','-','-','-','-','-'};   //C=charge   D=discharge '-' = none

    bool on_;

    const char programString[] PROGMEM = "ChCBSbBlDiFCStSBChDiCyCYChDiEB";
    void printProgram2chars(Program::ProgramType prog)
    {
        STATIC_ASSERT(sizeOfArray(programString)-1 == Program::LAST_PROGRAM_TYPE*2);
        //TODO: ??
        lcdPrint_P(programString+prog*2, 2);
    }
   
    uint8_t getChargeProcent()
    {
        uint16_t v1,v2, v;
        v2 = ProgramData::currentProgramData.getVoltage(ProgramData::VCharge);
        v1 = ProgramData::currentProgramData.getVoltage(ProgramData::ValidEmpty);
        v =  AnalogInputs::getRealValue(AnalogInputs::VoutBalancer);
        
        if(v >= v2) return 99;
        if(v <= v1) return 0;
        v-=v1;
        v2-=v1;
        v2/=100;
        v=  v/v2;
        if(v >= 100) v=99; //not 101% with isCharge
        return v;
    }
    
    AnalogInputs::ValueType getBalanceValue(uint8_t cell, AnalogInputs::Type type)
    {
        if(type == AnalogInputs::Voltage)
            return Balancer::getPresumedV(cell);
        return TheveninMethod::getReadableRthCell(cell);
    }

    void printBalancer(uint8_t cell, AnalogInputs::Type type) {
        if(AnalogInputs::isConnected(AnalogInputs::Name(AnalogInputs::Vb1+cell))) {
            lcdPrintAnalog(getBalanceValue(cell, type), type, 6);
        } else {
            lcdPrint_P(PSTR("  --  "));
        }
    }

    void displayBalanceInfo(uint8_t from, AnalogInputs::Type type)
    {
        
        lcdSetCursor0_0();
        if (blinkIcon)
        {
         blinkIcon = false;
        } else
        {
         blinkIcon = true;
        }
        
#ifdef KNIGHTRIDEREFFECT        
        if ( knightRiderDir==true) knightRiderCounter++; else knightRiderCounter--;
        if (knightRiderCounter>5) knightRiderDir=false;
        if (knightRiderCounter<=1) knightRiderDir=true;
#endif
        
        
        
        char c = ' ';
        if(!Balancer::isWorking()) {
            if(!Balancer::isStable())
                c = 'm';
        } else {
            if(Balancer::savedVon_)
                c = 'B';
            else
                c = 'b';
        }

        lcdPrintChar(c);

        if(Balancer::balance_ != 0) {
            uint8_t  j = 1;
            for(uint8_t i = 0; i < Balancer::getCells(); i++) {
                if(i == Balancer::minCell_) {
                  c = '_';
                } else {
                    if(Balancer::balance_&j) if (blinkIcon) c = '!'-34; else c =' ';
                    else c = '-';
                }
                lcdPrintChar(c);
                j<<=1;
            }
            lcdPrintSpaces(7 - Balancer::getCells());
            //lcdPrintChar(' ');
        }
        else 
        {
#ifdef KNIGHTRIDEREFFECT        
           char knightRiderArrow;
           if (knightRiderDir==true) knightRiderArrow='>'; else knightRiderArrow='<';
           if (c == 'm')
           {
             for (uint8_t i=1; i<7; i++ )
             {
              if (knightRiderCounter==i) lcdPrintChar(knightRiderArrow); else lcdPrintChar(' ');
             }
           lcdPrintChar(' ');
           }
           else
           {
           lcdPrintSpaces(7);
           }
#endif
#ifndef KNIGHTRIDEREFFECT  
        lcdPrintSpaces(7);
#endif
        }
        

        lcdPrintDigit(from+1);
        lcdPrintChar(':');
#ifdef ENABLE_B0_DISCHARGE_VOLTAGE_CORRECTION
        if(from == 0 && Discharger::isPowerOn() && Discharger::getValue()>0) {
            lcdPrint_P(PSTR("n.a."));
            from++;
        } else {
            printBalancer(from++, type);
        }
#else
        printBalancer(from++, type);
#endif
        lcdPrintSpaces();

        lcdSetCursor0_1();
        lcdPrintDigit(from+1);
        lcdPrintChar(':');
        printBalancer(from++, type);
        lcdPrintDigit(from+1);
        lcdPrintChar(':');
        printBalancer(from, type);
        lcdPrintSpaces();
    }

} // namespace Screen

void Screen::printCharge() {
    lcdPrintCharge(AnalogInputs::getRealValue(AnalogInputs::Cout), 8);
    lcdPrintChar(' ');
}

void Screen::printChar_Time() {
    char c = 'N';
    if(SMPS::isPowerOn()) {
        c = 'C';
    } else if(Discharger::isPowerOn()) {
        c = 'D';
        if(SMPS::isPowerOn()) c = 'E';
    } else if(Balancer::isWorking()) {
        c = 'B';
    }

    lcdPrintChar(c);
    lcdPrintChar(' ');
    lcdPrintTime(getTimeSec());
    lcdPrintChar(' ');
}

void Screen::displayScreenFirst()
{
    lcdSetCursor0_0();
    printCharge();
    AnalogInputs::printRealValue(AnalogInputs::Iout,     7);
    lcdPrintSpaces();

    lcdSetCursor0_1();
    printChar_Time();
    AnalogInputs::printRealValue(AnalogInputs::VoutBalancer,     7);
    lcdPrintSpaces();
}

void Screen::displayScreenCIVlimits()
{
    lcdSetCursor0_0();
    lcdPrintCharge(ProgramData::currentProgramData.getCapacityLimit(), 8);
    lcdPrintChar(' ');
    lcdPrintCurrent(TheveninMethod::getImax(), 7);
    lcdPrintSpaces();

    lcdSetCursor0_1();
    lcdPrint_P(PSTR("Limits: "));
    lcdPrintVoltage(TheveninMethod::Vend_, 7);
    lcdPrintSpaces();
}

uint16_t Screen::getTimeSec()
{
    uint32_t t = startTime_totalTime_;
    if(on_) t = Timer::getMiliseconds() - startTime_totalTime_;
    t/=1000;
    return t;
}

void Screen::powerOn()
{
    startTime_totalTime_ = Timer::getMiliseconds();
    totalBalanceTime_ = 0;
    totalChargDischargeTime_ = 0;
    on_ = true;
}

void Screen::powerOff()
{
    startTime_totalTime_ = Timer::getMiliseconds() - startTime_totalTime_;
    on_ = false;
}

void Screen::doSlowInterrupt()
{
   if(SMPS::isWorking() || Discharger::isWorking())
       totalChargDischargeTime_ += SLOW_INTERRUPT_PERIOD_MILISECONDS;

   if(Balancer::isWorking())
       totalBalanceTime_ += SLOW_INTERRUPT_PERIOD_MILISECONDS;

}

uint16_t Screen::getTotalChargDischargeTime()
{
    return (totalChargDischargeTime_/1000/60);
}


void Screen::displayScreenTime()
{
    lcdSetCursor0_0();
    lcdPrint_P(PSTR("time:     ")); lcdPrintTime(getTimeSec());
    lcdSetCursor0_1();
    lcdPrint_P(PSTR("b "));
    lcdPrintTime(totalBalanceTime_/1000);
    lcdPrint_P(PSTR("  "));
    lcdPrintTime(totalChargDischargeTime_/1000);
}


void Screen::displayScreenR()
{
    lcdSetCursor0_0();
    lcdPrint_P(PSTR("batt. R="));
    lcdPrintResistance(TheveninMethod::getReadableBattRth(), 8);
    lcdPrintSpaces();
    lcdSetCursor0_1();
    if(AnalogInputs::isConnected(AnalogInputs::Vbalancer)) {
        lcdPrint_P(PSTR("wires R="));
        lcdPrintResistance(TheveninMethod::getReadableWiresRth(),8);
    }
    lcdPrintSpaces();
}

void Screen::displayScreenVinput()
{
    lcdSetCursor0_0();
    lcdPrint_P(PSTR("Vinput="));
    AnalogInputs::printRealValue(AnalogInputs::Vin, 7);
    lcdPrintSpaces();
    lcdSetCursor0_1();
    lcdPrint_P(PSTR(" limit="));
    lcdPrintAnalog(settings.inputVoltageLow_, AnalogInputs::Voltage, 7);
    lcdPrintSpaces();
}

void Screen::displayScreenVout()
{
    lcdSetCursor0_0();
    lcdPrint_P(PSTR("Vout ="));
    AnalogInputs::printRealValue(AnalogInputs::Vout, 7);
    lcdPrintSpaces();
    lcdSetCursor0_1();
    lcdPrint_P(PSTR("Vbal.="));
    AnalogInputs::printRealValue(AnalogInputs::Vbalancer, 7);
    lcdPrintSpaces();
}

void Screen::displayScreenTemperature()
{
    lcdSetCursor0_0();
    lcdPrint_P(PSTR("Text="));
    if(settings.externT_)
        AnalogInputs::printRealValue(AnalogInputs::Textern,    5);
    else
        lcdPrint_P(PSTR("-"));
    lcdPrintSpaces();

    lcdSetCursor0_1();
#ifdef ENABLE_T_INTERNAL
    lcdPrint_P(PSTR("Tint="));
    AnalogInputs::printRealValue(AnalogInputs::Tintern,    5);
#endif
    lcdPrintSpaces();
}

void Screen::displayStrings(const char *s1, const char *s2)
{
    lcdClear();
    lcdSetCursor0_0(); lcdPrint_P(s1);
    lcdSetCursor0_1(); lcdPrint_P(s2);
}

namespace {
    void screenEnd(const char * firstLine) {
        lcdSetCursor0_0();
        lcdPrint_P(firstLine);
        lcdSetCursor0_1();
        if(strlen_P(Program::stopReason_)>0) {
            lcdPrint_P(PSTR("reason: "));
            lcdPrint_P(Program::stopReason_);
        }
        lcdPrintSpaces();
    }
}

void Screen::displayScreenProgramCompleted()
{
    screenEnd(PSTR("program complete"));
    lcdSetCursor0_1();
    lcdPrint_P(PSTR("Time: ")); lcdPrintTime(getTimeSec());
}

void Screen::displayMonitorError()
{
    screenEnd(PSTR("monitor error"));
}

namespace {
    void deltaV() {
        int x = AnalogInputs::getRealValue(AnalogInputs::deltaVout);
        lcdPrintSigned(x, 5);
        lcdPrintChar('m');
        lcdPrintChar('V');
        lcdPrintSpaces();

    }
    void deltaT() {
        if(settings.externT_) {
            int x = AnalogInputs::getRealValue(AnalogInputs::deltaTextern);
            lcdPrintSigned(x*10, 5);
            lcdPrintChar('m');
            lcdPrintChar('C');
        } else {
            lcdPrint_P(PSTR("not used"));
        }
        lcdPrintSpaces();
    }
}

void Screen::displayDeltaFirst()
{
    lcdSetCursor0_0();
    printCharge();
    deltaT();

    lcdSetCursor0_1();
    printChar_Time();
    deltaV();
}

void Screen::displayScreenCycles()
{
   uint8_t c;
   //multiscreen (5x2 cyclenumber, C/D, timeC/timeDC, mAhCh/mAhDC)
   toggleTextCounter++; if (toggleTextCounter>5) toggleTextCounter=0;
  
   if ( toggleTextCounter==5){ toggleTextCycleCounter_++ ;if (toggleTextCycleCounter_ > (settings.CDcycles_*2)) toggleTextCycleCounter_ = 1;}
   
   c=toggleTextCycleCounter_-1;
   lcdSetCursor0_0();
   lcdPrintUnsigned(toggleTextCycleCounter_, 2);
   lcdPrintSpaces(1);
   lcdPrintChar(cyclesHistoryMode[c]);
   lcdPrintSpaces(1);
   lcdPrintTime(cyclesHistoryTime[c]);
   lcdPrintSpaces();
   lcdSetCursor0_1();
   lcdPrintSpaces(1);
     if (cyclesHistoryMode[c] == 'C')
     {
      lcdPrintCharge(cyclesHistoryChCapacity[c],8); 
     }
     else if (cyclesHistoryMode[c] == 'D')
     {
      lcdPrintCharge(cyclesHistoryDcCapacity[c],8); 
     }
   lcdPrintSpaces();
}

void Screen::displayDeltaVout()
{
    lcdSetCursor0_0();
    lcdPrint_P(PSTR("maxVout="));
    AnalogInputs::printRealValue(AnalogInputs::deltaVoutMax, 7);
    lcdPrintSpaces();

    lcdSetCursor0_1();
    lcdPrint_P(PSTR("delta V= "));
    deltaV();
}

void Screen::displayScreenEnergy()
{   //TODO_NJ
    //this is multiscreen
    toggleTextCounter++; if (toggleTextCounter>20) toggleTextCounter=0;
    
    //get procent increment time
    getDeltaProcentTimeSec();
    //TODO_NJ_end  
    
     
    if (toggleTextCounter<10)
    {
      lcdSetCursor0_0();
      printCharge();
      AnalogInputs::printRealValue(AnalogInputs::Iout, 7);
       
      lcdPrintSpaces();
      lcdSetCursor0_1();
      printChar_Time();
      lcdPrint_P(PSTR(" "));
    }
    else
    {
      lcdSetCursor0_0();
      AnalogInputs::printRealValue(AnalogInputs::Pout, 8);
      lcdPrint_P(PSTR(" "));
      AnalogInputs::printRealValue(AnalogInputs::Iout, 7);
      lcdPrintSpaces();
      lcdSetCursor0_1();
      AnalogInputs::printRealValue(AnalogInputs::Eout, 8);
      lcdPrint_P(PSTR("  "));
    }
 

 
 
    if (toggleTextCounter<10 && SMPS::isPowerOn() )
    { //display calculated simple ETA
       
   
      if(etaSecLarge>20)  //bigger 20sec for ETA calc (is 1C)
      {
        lcdPrintTime(getETATime()); //TODO_NJ (not accurate for balancing time)
      }
      else 
      {
        lcdPrint_P(PSTR(" --:--")); 
      }
    
    
    
    }
    else
    {  
      lcdPrintUnsigned(procent, 4);
      lcdPrint_P(PSTR("%"));  
      lcdPrintSpaces();
    }
    
}

#ifdef SCREENANIMATION
void Screen::displayAnimation()
{

    for (uint8_t i=0; i<16; i++)
    {
      
      lcdSetCursor(15-i,1);
      lcdPrintChar('!'-34);
      lcdSetCursor(i,0);
      lcdPrintChar('!'-34);
      hardware::delay(10); 
    }
     hardware::delay(150);

}
#endif

void Screen::displayDeltaTextern()
{
    lcdSetCursor0_0();
    lcdPrint_P(PSTR("Text="));
    if(settings.externT_) {
        lcdPrintTemperature(AnalogInputs::deltaLastT_, 9);
    } else {
        lcdPrint_P(PSTR("not used"));
    }
    lcdPrintSpaces();

    lcdSetCursor0_1();
    lcdPrint_P(PSTR("delta T= "));
    deltaT();
}


void Screen::displayNotImplemented()
{
    displayStrings(PSTR("Function not"), PSTR("implemented yet"));
}


void Screen::runNotImplemented()
{
    displayNotImplemented();
    waitButtonPressed();
}

void Screen::runCalibrateBeforeUse()
{
    displayStrings(PSTR("please calibrate"), PSTR("before use"));
    Timer::delay(5000);
}


void Screen::displayScreenReversedPolarity()
{
    lcdSetCursor0_0();
    lcdPrint_P(PSTR("REVERSE POLARITY"));
}


void Screen::displayWarningScreen()
{
   hardware::delay(500);
//                               1234567890123456
    Screen::displayStrings(PSTR("    WARNING"),
                           PSTR("Balancer not.con"));
   
    hardware::delay(900);
    Screen::displayStrings(PSTR("  hold button"),
                           PSTR(" to Start/stop"));
    hardware::delay(900);                       
}

void Screen::displayCalibrationErrorScreen()
{
//                                 1234567890123456
      Screen::displayStrings(PSTR("  Calibration"),
                             PSTR("     ERROR"));
      hardware::delay(8000);
}


void Screen::displayStartInfo()
{   
    
    resetETA();
    //resetCycleHistory();
    
    
    lcdSetCursor0_0();
    ProgramData::currentProgramData.printBatteryString(4);
    lcdPrintChar(' ');
    ProgramData::currentProgramData.printVoltageString();
    lcdPrintChar(' ');
    printProgram2chars(Program::programType_);

    lcdSetCursor0_1();
    procent = getChargeProcent();
    lcdPrintUnsigned(procent, 2);
    lcdPrint_P(PSTR("% "));
    
    
    

    int bindex = blink.getBlinkIndex();
    if(bindex & 1) AnalogInputs::printRealValue(AnalogInputs::Vout, 5);
    else lcdPrintSpaces(5);

    lcdPrintChar(' ');
    if(ProgramData::currentProgramData.isLiXX()) {
        //display balance port
        if(bindex & 2) AnalogInputs::printRealValue(AnalogInputs::Vbalancer, 5);
        else lcdPrintSpaces(5);

        if(bindex & 4) lcdPrintDigit(AnalogInputs::getConnectedBalancePorts());
        else lcdPrintChar(' ');
    } else {

        lcdPrintCharge(ProgramData::currentProgramData.battery.C, 6);
        lcdPrintSpaces();
    }
}

void Screen::resetETA()
{
//reset ETA
    etaSec=0;
    timeSecOldETACalc=0;
    procent_=procent;
    etaSecLarge = 0;

}

void Screen::resetCycleHistory()
{
  for (uint8_t i=0; i<10; i++)
  {
    cyclesHistoryMode[i] = '-';  
    cyclesHistoryTime[i] = 0;
  }

}

void Screen::getDeltaProcentTimeSec()
{
    procent = getChargeProcent();
    if(procent_ < procent)
    {
      procent_=procent; //probable only 1% max incremet/running
      etaSec = getTimeSec()-timeSecOldETACalc;
      timeSecOldETACalc = getTimeSec();
      if ((etaSec > etaSecLarge) || (etaSecLarge==0) )  { etaSecLarge=etaSec;}  // find longer time for deltaprocent
    } 
}

uint16_t Screen::getETATime()
{
    getDeltaProcentTimeSec();
    uint8_t kx;
 if( (AnalogInputs::getConnectedBalancePorts() == 0) && (absDiff(AnalogInputs::getRealValue(AnalogInputs::Vout),
             AnalogInputs::getRealValue(AnalogInputs::Vbalancer)) > ANALOG_VOLT(0.5)  ))
             {
             //balancer not connected
             kx=100;
             }
             else
             {
             //balancer connected
             kx=105; //plus 5% for balancing time (not accurate the oldiest lipo battery)
             }


    return (etaSecLarge*(kx-procent_));
}


void Screen::display(ScreenType screen)
{
    blink.incBlinkTime();
    switch(screen) {
    case ScreenFirst:                   return displayScreenFirst();
    case ScreenCIVlimits:               return displayScreenCIVlimits();
    case ScreenTime:                    return displayScreenTime();
    case ScreenTemperature:             return displayScreenTemperature();
    case ScreenBalancer1_3:             return displayBalanceInfo(0, AnalogInputs::Voltage);
    case ScreenBalancer4_6:             return displayBalanceInfo(3, AnalogInputs::Voltage);
    case ScreenBalancer1_3Rth:          return displayBalanceInfo(0, AnalogInputs::Resistance);
    case ScreenBalancer4_6Rth:          return displayBalanceInfo(3, AnalogInputs::Resistance);
    case ScreenStartInfo:               return displayStartInfo();
    case ScreenR:                       return displayScreenR();
    case ScreenVout:                    return displayScreenVout();
    case ScreenVinput:                  return displayScreenVinput();
    case ScreenEnergy:                  return displayScreenEnergy();
    case ScreenDeltaVout:               return displayDeltaVout();
    case ScreenDeltaTextern:            return displayDeltaTextern();
    case ScreenDeltaFirst:              return displayDeltaFirst();
    case ScreenCycles:                  return displayScreenCycles();
    }
}
