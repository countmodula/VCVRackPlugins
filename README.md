![alt text](./img/CountModulaLogo.png "Count Modula")
<h1>Count Modula</h1>
<h2>Plugin modules for VCV Rack v1.0 by Adam Verspaget (Count Modula)</h2>

<h3>Licenses</h3>

All source code in this repository is licensed under BSD-3-Clause by Adam Verspaget/Count Modula

All graphics including the Count Modula logo, panels and components are copyright © 2019 Adam Verspaget/Count Modula and may not be used in derivative works.

<h3>Donate</h3>
Whilst these modules are offered free of charge, if you like them or are using them to make money, please consider a small donation to The Count for the effort.
<p>&nbsp</p>
<a href="https://www.paypal.me/CountModula" target="_donate"><img src="https://www.paypalobjects.com/en_AU/i/btn/btn_donateCC_LG.gif" border="0" alt="Donate with PayPal"/></a>

<h3>Stay Informed</h3>
Follow Count Modula on facebook:
<p>&nbsp</p>
<a href="https://www.facebook.com/CountModula/"><img src="./img/facebook.png" alt="Count Modula on facebook"></a>

<h3>Modules - Release 1.1.0</h3>

<a href="MANUAL.md">Module Manual</a>

<ul>
<li><a href="./MANUAL.md#ASR">Analogue Shift Register</a>
<li><a href="./MANUAL.md#Attenuator">Attenuator</a>
<li><a href="./MANUAL.md#BinarySequencer">Binary Sequencer</a> *New features in v1.0.0
<li><a href="./MANUAL.md#BooleanLogic">Boolean Logic Modules</a>
	<ul>
		<li>AND
		<li>OR
		<li>VC Inverter
		<li>XOR
	</ul>
<li><a href="./MANUAL.md#Comparator">Comparator</a>
<li><a href="./MANUAL.md#EventArranger">Event Arranger</a>
<li><a href="./MANUAL.md#GateDelay">Gate Delay</a>
<li><a href="./MANUAL.md#GateModifier">Gate Modifier</a> * New in v1.1.0
<li><a href="./MANUAL.md#G2T">G2T</a>
<li><a href="./MANUAL.md#Inverter">Inverter</a>
<li><a href="./MANUAL.md#Mangler">Mangler</a>
<li><a href="./MANUAL.md#ManualCV">Manual CV</a>
<li><a href="./MANUAL.md#ManualGate">Manual Gate</a>
<li><a href="./MANUAL.md#MatrixMixer">Matrix Mixer</a>
<li><a href="./MANUAL.md#MinimusMaximus">Minimus Maximus</a>
<li><a href="./MANUAL.md#Mixer">Mixer</a>
<li><a href="./MANUAL.md#MorphShaper">Morph Shaper</a>
<li><a href="./MANUAL.md#Multiplexer">Multiplexer</a>
<li><a href="./MANUAL.md#Mute">Mute</a>
<li><a href="./MANUAL.md#Mute-iple">Mute-iple</a>
<li><a href="./MANUAL.md#PolyrhythmicGenerator">Polyrhythmic Generator</a>
<li><a href="./MANUAL.md#Rectifier">Rectifier</a> * New in v1.1.0
<li><a href="./MANUAL.md#SampleAndHold">Sample & Hold</a> * New in v1.1.0
<li><a href="./MANUAL.md#ShepardGenerator">Shepard Generator</a>
<li><a href="./MANUAL.md#SRFlipFlop">SR Flip Flop</a>
<li><a href="./MANUAL.md#StepSequencer8">8 Step Sequencer</a> * New in v1.1.0
<li><a href="./MANUAL.md#TappedGateDelay">Tapped Gate Delay</a>
<li><a href="./MANUAL.md#SRFlipFlop">T Flip Flop</a> * New in v1.1.0
<li><a href="./MANUAL.md#TriggerSequencer8">Trigger Sequencer (8 Step)</a>
<li><a href="./MANUAL.md#TriggerSequencer16">Trigger Sequencer (16 Step)</a>
<li><a href="./MANUAL.md#VCFrequencyDivider">Voltage Controlled Frequency Divider</a>
<li><a href="./MANUAL.md#VCPolarizer">Voltage Controlled Polarizer</a>
<li><a href="./MANUAL.md#VCSwitch">Voltage Controlled Switch</a>
<li><a href="./MANUAL.md#CGS">CGS Based Modules</a>
	<ul>
		<li><a href="./MANUAL.md#CVSpreader">CV Spreader</a>
		<li><a href="./MANUAL.md#BurstGenerator">Burst Generator</a>
	</ul>
</ul>

<h4>Still running Rack v0.6?</h4>
Version v0.6.3 of the Count Modula suite of modules can be found here:
<a href="https://github.com/countmodula/VCVRackPlugins/tree/V0.6">https://github.com/countmodula/VCVRackPlugins/tree/V0.6</a>

<h3>Release Log</h3>
<table>
	<tr valign="top">
		<th align="center">Date</th>
		<th align="center">Release</th>
		<th align="left">Notes</th>
	</tr>
	<tr valign="top">
		<td align="center">XX-XXX-2019</td>
		<td align="center">v1.1.0</td>
		<td align="left">
			<b>New Modules</b><br/>
			<ul>
				<li>Gate Modifier - Extend or shorten gate/trigger signals.
				<li>Rectifier - Full and half wave rectification around a CV controlled axis.
				<li>Sample & Hold - Sample/Track and hold.
				<li>T Flip Flop - Toggle style flip flop.
				<li>Mangler - Voltage controlled sample rate and bit depth reducer.
				<li>8 Step Sequencer - A classic 8 step Gate/CV Sequencer.
			</ul>
			<b>Module Updates</b><br/>
			<ul>
				<li>Manual Gate - Current state is now preserved when saving and re-opening a patch.
				<li>Mute - Current state is now preserved when saving and re-opening a patch.
				<li>SR Flip Flop - Current state is now preserved when saving and re-opening a patch.
				<li>Trigger Sequencers - Addition of length indicators.
			</ul>
			<b>Issues Resolved:</b><br/>
			<ul>
				<li>Issue #18 Blue knob colour lightened for better visibility.
			</ul>
		</td>
	</tr>
	<tr valign="top">
		<td align="center">21-Jun-2019</td>
		<td align="center">v1.0.2</td>
		<td align="left">
			<b>Module Updates</b><br/>
			<ul>
				<li>Attenuator - Attenuation of polyphonic signals
				<li>Inverter - Inversion of polyphonic signals
				<li>Mute - Muting of polyphonic signals
				<li>Mut-iple - Muting of polyphonic signals. Hard/Soft mute option.
				<li>PolyrhythmicGenerator  - Added polyphonic output containing all channels
				<li>Shepard Generator - Saw and Tri waveforms available as polyphonic signals
				<li>Voltage Controlled Polarizer - polarization of polyphonic signals
				<li>Voltage Controlled Switch - switching of polyphonic signals
			</ul>
		</td>
	</tr>
	<tr valign="top">
		<td align="center">19-Jun-2019</td>
		<td align="center">v1.0.1</td>
		<td align="left">
			<b>New Modules</b><br/>
			<ul>
				<li>Utility Mixer
				<li>Mute
			</ul>
		</td>
	</tr>
	<tr valign="top">
		<td align="center">16-Jun-2019</td>
		<td align="center">v1.0.0</td>
		<td align="left">
			Port from Rack v0.6 to v1.0
		</td>
	</tr>
	<tr valign="top">
		<td align="center">09-Jun-2019</td>
		<td align="center">v0.6.2 and v0.6.3</td>
		<td align="left">
			<b>New Modules</b><br/>
			<ul>
				<li>Multipexer
				<li>8 Step Trigger Sequencer
				<li>16 Step Trigger Sequencer
			</ul>
			<b>Issues Resolved:</b><br/>
			<ul>
				<li>Issue #6 Binary Sequencer enhanced to split run and reset inputs.
				<li>Issue #7 Randomize not setting switch values correctly.
			</ul>
		</td>
	</tr>
	<tr valign="top">
		<td align="center">04-Jun-2019</td>
		<td align="center">v0.6.1</td>
		<td align="left">
			<b>Issues Resolved:</b><br/>
			Issue #3 svgs incorrectly packaged.<br/>
		</td>
	</tr>
	<tr valign="top">
		<td align="center">03-Jun-2019</td>
		<td align="center">v0.6.0</td>
		<td>Initial Release</td>
	</tr>
</table>



