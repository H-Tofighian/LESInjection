# LES particle injection model

use flowing code in your CloudProperties file:

        model70
        {
            type            patchInjection;
            patchName       inlet;
            massTotal       0.0025;
            parcelBasisType mass;
            SOI             0;
            duration        100;
            parcelsPerSecond 2000000;
            U0              (3.5 0 0);// if si==0 then u_p = U0
            randomise       true;
            flowRateProfile 1;
            sizeDistribution
            {   
                type    fixedValue;
                fixedValueDistribution
                {
                    value 70e-6;
                }
            }
            ksgs  0;//k_SGS for dispersion models (if you use it) 
            si 0.95;//u_p = u_fliud * si
        }
