# Bitcoin Price Prediction using Monte Carlo Simulation and HPC

## Overview
This project implements a **Monte Carlo Simulation (MCS)** framework to forecast **Bitcoin price dynamics** using the **Geometric Brownian Motion (GBM)** model.  

To handle the **massive number of simulation paths** required for reliable forecasting, the project leverages **High-Performance Computing (HPC)** techniques:  
- **MPI (Message Passing Interface)** for **inter-node parallelism**  
- **OpenMP** for **intra-node multithreading**  

The combination enables simulation of **millions to billions of paths**, making large-scale stochastic forecasting feasible.

---

## Motivation
Bitcoin is highly volatile and difficult to predict deterministically. Instead of producing a single-point forecast, **Monte Carlo simulations generate a distribution of possible future prices**, allowing us to study:
- Expected values  
- Confidence intervals  
- Tail risks (e.g., Value-at-Risk, Conditional VaR)  

---

## Methodology

### 1. Model: Geometric Brownian Motion (GBM)
The GBM model describes the price evolution as:

$$
dS_t = \mu S_t \, dt + \sigma S_t \, dW_t
$$

Discretized for simulation:

$$
S_{t+\Delta t} = S_t \cdot \exp \Big[ (\mu - 0.5\sigma^2)\Delta t + \sigma \sqrt{\Delta t} \, Z \Big], \quad Z \sim \mathcal{N}(0,1)
$$

---

### 2. Parameter Estimation
- Compute **log returns** from historical Bitcoin prices  
- Estimate drift ($\mu$) and volatility ($\sigma$)  
- Use these in the GBM update equation  

---

### 3. Monte Carlo Simulation
- Generate $M$ independent paths over horizon $T$  
- Aggregate statistics: mean, variance, quantiles, histograms  
- Visualize with fan charts and terminal price distributions  

---

##  HPC Implementation

### Sequential C++ (Baseline)
- Straightforward loop over all paths  
- Correct but **computationally slow** for large $M$  

### Parallelization
- **MPI** → distributes paths across processes (**inter-node**)  
- **OpenMP** → parallelizes within each process (**intra-node**)  
- **Hybrid MPI + OpenMP** → combines both for maximum scalability  

