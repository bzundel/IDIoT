#include <stdio.h>
#include "shell.h"
#include "saul_reg.h"
#include <phydat.h>
#include "unistd.h"
#include "math.h"
#include "unistd.h"

int main(void)
{
    saul_reg_t* reg = saul_reg_find_type(SAUL_SENSE_TEMP);
    //saul_reg_t* reg = saul_reg_find_nth(1);

    sleep(3);

    phydat_t data;
    data.unit = UNIT_TEMP_C;

    while (1) {
        saul_reg_read(reg, &data);

        printf("%.2f\n", ((int)data.val[0]) * pow(10, data.scale));
        
        sleep(3);
    }

    return 0;
}
