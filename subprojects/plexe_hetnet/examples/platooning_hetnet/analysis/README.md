Baseline comparison
===

**Simulations to run**
* ACCOnly
* ArtificialFailures

**Parsing output files and plotting the results**
```
make ArtificialFailuresDynamics.Rdata ArtificialFailuresEvents.Rdata ACCOnlyDynamics.Rdata ACCOnlyEvents.Rdata
cd dynamics-plots
Rscript plot-dynamics.R
Rscript plot-legend.R
```

Multiple artificial failures
===

**Simulations to run**
* MultipleArtificialFailures
* SimultaneousArtificialFailures

**Parsing output files and plotting the results**
```
make MultipleArtificialFailuresDynamics.Rdata MultipleArtificialFailuresEvents.Rdata SimultaneousArtificialFailuresDynamics.Rdata SimultaneousArtificialFailuresEvents.Rdata
cd dynamics-plots
Rscript plot-dynamics.R
Rscript plot-legend.R
```

Realistic failures
===

**Simulations to run**
* InterferenceScenario

**Parsing output files and plotting the results**
```
make InterferenceScenarioHandovers.Rdata InterferenceScenarioDynamics.Rdata InterferenceScenarioEvents.Rdata InterferenceScenarioFER.Rdata
cd dynamics-plots
Rscript plot-dynamics.R
Rscript plot-fer.R
Rscript plot-legend.R
```

Validations
===

**Simulations to run**
* Validation

**Parsing output files and plotting the results**
```
make ValidationDynamics.Rdata
cd dynamics-plots
Rscript plot-validation.R
Rscript plot-legend.R
```
