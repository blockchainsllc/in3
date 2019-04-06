msg_type_t msg_get_type(char *c);
action_type_t msg_get_action(char *c);
char *msg_get_payload(struct in3_client *c);
int verify_rent(struct in3_client *c);
void do_action(action_type_t action);
