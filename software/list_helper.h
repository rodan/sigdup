#ifndef __list_helper_h__
#define __list_helper_h__

struct list_struct {
    void *data;
    struct list_struct *next;
};

struct list_struct *list_init(struct list_struct **head, struct list_struct **curr, void *data);
struct list_struct *list_add_node(struct list_struct **head, struct list_struct **curr, void *data);
int list_get_head(struct list_struct **head, void **data);
int list_del_head(struct list_struct **head);
int list_rm_node(struct list_struct **head, struct list_struct **curr, void *data);
//void list_print(struct list_struct *head);
int list_len(struct list_struct *head);
int list_free(struct list_struct **head);
int list_non_empty(struct list_struct *head);

#endif
