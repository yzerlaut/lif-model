/*
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*

 leaky-integrate-and-fire model 
 implemented by Yann Zerlaut, Istituto Italiano di Tecnologia, Italy
              contact: yann.zerlaut@iit.it


 */

#include "lif-model.h"
#include <iostream>
#include <main_window.h>

extern "C" Plugin::Object*
createRTXIPlugin(void)
{
  return new LifModel();
}

static DefaultGUIModel::variable_t vars[] = 
  {
	{ "Vm", "Membrane Potential (V)", DefaultGUIModel::OUTPUT, },
	{ "Istim", "Input current (A)", DefaultGUIModel::INPUT, },
	{ "Ipulse (pA)", "Amplitude of Applied Current (pA)",
		DefaultGUIModel::PARAMETER, },
	{ "Tpulse (ms)", "Pulse Duration (ms)",
		DefaultGUIModel::PARAMETER, },
	{ "Cm (pF)", "Membrane capacitance (pF)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Gl (nS)", "Leak conductance (nS)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "El (mV)", "Leak reversal potential (mV)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Vt (mV)", " spiking threshold  (mV)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Vp (mV)", " spiking peak (mV)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Tr (ms)", "refractory period (ms)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Rate (Hz)", "Rate of integration (Hz)", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::UINTEGER, },
	{ "Time (s)", "Time (s)", DefaultGUIModel::STATE, },
  };

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);
  

LifModel::LifModel(void)
  : DefaultGUIModel("LIF-Model", ::vars, ::num_vars)
{
  initParameters();
  // setWhatsThis("<p><b>LifModel:</b><br>Leaky-Integrate-and-Fire model</p>");
  DefaultGUIModel::createGUI(vars,
                             num_vars); // this is required to create the GUI
  update(INIT); // into the constructor
  refresh();    // this is required to update the GUI with parameter and state values
  QTimer::singleShot(0, this, SLOT(resizeMe()));
}

LifModel::~LifModel(void)
{
}

void
LifModel::execute(void)
  {
    double Ipulse2;
      
    /* EVERYTHING SHOULD BE TRANSLATED T0 SI UNITS HERE */    
    systime = count * period; // time in seconds for display

    if ((count%(2*count_pulse))<count_pulse)
      { Ipulse2 = Ipulse; } else {Ipulse2 = 0;}
      
    if (systime-last_spike<=Tr*1e-3) {V = El*1e-3 ;}
    else if (V>Vt*1e-3) {V = Vp*1e-3 ; last_spike = systime ;} 
    else {
      for (int i = 0; i < steps; ++i)
	{
	  V += period/steps/(Cm*1e-12)*(
					Ipulse2*1e-12 +
					input(0) +
					(1e-9*Gl)*( (El*1e-3)-V) );
	}
    }
    
    output(0) = V; // 
    count++;
    
  }

void
LifModel::update(DefaultGUIModel::update_flags_t flag)
{
  switch (flag) {
    case INIT:
      period = RT::System::getInstance()->getPeriod() * 1e-9; // s
      setParameter("Ipulse (pA)", QString::number(Ipulse));
      setParameter("Tpulse (ms)", QString::number(Tpulse));
      setParameter("Cm (pF)", QString::number(Cm));
      setParameter("Gl (nS)", QString::number(Gl));
      setParameter("El (mV)", QString::number(El)); 
      setParameter("Vt (mV)", QString::number(Vt)); 
      setParameter("Vp (mV)", QString::number(Vp)); 
      setParameter("Tr (ms)", QString::number(Tr)); 
      setParameter("Rate (Hz)", QString::number(rate)); 
      break;

    case MODIFY:
      Ipulse = getParameter("Ipulse (pA)").toDouble();
      Tpulse = getParameter("Tpulse (ms)").toDouble();
      Cm = getParameter("Cm (pF)").toDouble();
      Gl = getParameter("Gl (nS)").toDouble();
      El = getParameter("El (mV)").toDouble();
      Vt = getParameter("Vt (mV)").toDouble();
      Vp = getParameter("Vp (mV)").toDouble();
      Tr = getParameter("Tr (ms)").toDouble();
      rate = getParameter("Rate (Hz)").toDouble();
      steps = static_cast<int> (ceil(period * rate));
      V = El;
      count_pulse = static_cast<int> (ceil(Tpulse*1e-3/period));
      break;

    case UNPAUSE:
      break;

    case PAUSE:
      break;

    case PERIOD:
      period = RT::System::getInstance()->getPeriod() * 1e-9; // in s
      break;

    default:
      break;
  }
}

void
LifModel::initParameters(void)
{
  Ipulse = 0.0; // pA
  Tpulse = 500.0; // ms
  Cm = 200.0; // pF
  Gl = 10.0; // pS
  El = -70.0; // mV
  rate = 20000; // 20kHz per default here
  Vt = -50.0 ;
  Vp = 10.0 ;
  Tr = 5.0 ;
  last_spike = -1000.0 ;
  
  V = El*1e-3; // initialize to rest
	
  count = 0;
  systime = 0;
  period = RT::System::getInstance()->getPeriod() * 1e-9; // time in s
  // calculate how many integrations to perform per execution step
  steps = static_cast<int> (ceil(period * rate));
  count_pulse = static_cast<int> (ceil(Tpulse*1e-3/period));
}

