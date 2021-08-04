#include "./module.h"




STATIC const mp_rom_map_elem_t cfleds_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_cfleds)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_RmtLed), (mp_obj_t)&ledmodule_rmtledType}
};
STATIC MP_DEFINE_CONST_DICT(cfleds_module_globals, cfleds_module_globals_table);

const mp_obj_module_t cfleds_user_cmodule = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&cfleds_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_cfled, cfleds_user_cmodule, 1);