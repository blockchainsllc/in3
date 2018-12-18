msg_type_t msg_get_type(struct in3_client *c);
action_type_t msg_get_action(struct in3_client *c);
char *msg_get_payload(struct in3_client *c);
int verify_rent(struct in3_client *c);
void do_action(action_type_t action);
