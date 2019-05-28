![alt text](./img/CountModulaLogo.png "Count Modula")
<h1>Count Modula</h1>
<h2>Plugin modules for VCV Rack by Adam Verspaget (Count Modula)</h2>
<hr/>
<h3>Analogue Shift Register</h3>
<table>
<tr valign="top">
<td width=150><img src="./img/ASR.png"></td> 
<td>A dual 4 output or single 8 output shift register. On each the rising edge at the shidt input, the signal present at each output is propagated to the next successive output and the input is simultaneously sampled and sent to the first output. The shift input on channel 2 is normalled to the shift input on channel 1 and the signal input on channel 2 is normalled to output 4 of channel 1 so that, with no cables connected to channel 2, the module functions as a single 8 output shift register.</td>
</tr>
</table>

<h3>Attenuator</h3>
<table>
<tr valign="top">
<td width=100><img src="./img/Attenuator.png"></td> 
<td>A basic dual attenuator with switchable attenuverting capability on the top channel. With no CV input the module will output control voltage between 0 and 10V (top channel -10V and +10V when in attenueverter mode) proportional to the position of the level knob.</td>
</tr>
</table>

<h3>Binary Sequencer</h3>
<table>
<tr valign="top">
<td width=220><img src="./img/BinarySequencer.png"></td> 
<td>Similar to a now discontinued Frac format module, this is a binary counter based sequencer where the individual bits of the counter are mixed together in varying  proportions to produce a repeating CV pattern. The output can be smoothed into slowly varying voltages with the Lag and Lag Shape controls and the outpur range cab be set to 2, 5 or 10 volts. The sequencer can be internally or externally clocked and offers both gate and trigger outputs</td>
</tr>
</table>

<h3>Boolean Logic Modules</h3>
<table>
<tr valign="top">
<td width=260><img src="./img/BooleanLogic.png"></td> 
<td>
<ul>
<li>AND: A quad input AND/NAND gate with built in NOT gate (logical inverter). The AND output is high if all connected inputs are also high. With nothing connected to the Inverter input (I), the NOT output will perform the NAND function.
<li>OR: A quad input OR/NOR gate with built in NOT gate (logical inverter). The OR output is high if any connected input is high. With nothing connected to the Inverter input (I), the NOT output will perform the NOR function.
<li>VC Inverter: a logical inverter with voltage control over the invert function. Will only invert if the enable input is High. 
<li>XOR: A quad input XOR/XNOR gate with built in NOT gate (logical inverter). The XOR output is high if only one connected input is high. With nothing connected to the Inverter input (I), the NOT output will perform the XNOR function.
</ul>
</td>
</tr>
</table>

<h3>Inverter</h3>
<table>
<tr valign="top">
<td width=100><img src="./img/Inverter.png"></td> 
<td>A quad voltage inverter. Unlike the Boolean Logic Inverter, this module inverts around zero. Positive voltage in become a negative voltage out and vice-versa.</td>
</tr>
</table>

<h3>Gate Delay</h3>
<table>
<tr valign="top">
<td width=240><img src="./img/GateDelay.png"></td> 
<td>A dual gate delay line giving up to 40 seconds of delay with voltage control over the delay time. Note that switching from a shorter delay time range to a longer one may introduce time travel artefacts were a gate that has already been output may be output again at the end of the new longer delay time depending on where it is in the pipeline. This is due to the way the delay line functions and is normal behaviour.Three gate outputs are available with the Direct output following the gate input, the Delay output providing only the delayed gates middle output providing a logical mix of the two.</td>
</tr>
</table>

<h3>Manual CV</h3>
<table>
<tr valign="top">
<td width=100><img src="./img/ManualCV.png"></td> 
<td>A simple dual manual CV generator offering +/- 10V with both coarse and fine controls.</td>
</tr>
</table>



