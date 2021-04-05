#include <ngx_http.h>
#include <ngx_config.h>
#include <ngx_core.h>

/*
#include <arpa/inet.h>
#include <netinet/in.h>
*/
#define ENABLE_RBTREE	1

static char *ngx_http_count_module_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
// curl http://192.168.142.129:8888/king
static ngx_int_t ngx_http_count_module_handler(ngx_http_request_t *r);

// 模块里面的指令集
// 解析到count后就会执行set函数
static ngx_command_t count_commands[] = {
    {
		ngx_string("count"),
		NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS, // 放在哪个小模块中
		ngx_http_count_module_set,
		NGX_HTTP_LOC_CONF_OFFSET,
		0, NULL
	},
	ngx_null_command
};
//hadler模块     为什么统计只能使用handler模块不能使用其它模块，在接收到请求的时候就开始处理
//filter模块     过滤器 后端返回数据时，在返回的信息上添加处理
//upstream模块 转发用

// 实际是处理配置文件的
static ngx_http_module_t count_ctx = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

//ngx_http_count_module 
ngx_module_t ngx_http_count_module = {
	NGX_MODULE_V1,
	&count_ctx,
	count_commands,
	NGX_HTTP_MODULE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NGX_MODULE_V1_PADDING
};

typedef struct {
	int count; //count
	struct in_addr addr; //ip
} ngx_pv_table;

ngx_pv_table pv_table[256];

#if ENABLE_RBTREE
ngx_rbtree_t ngx_pv_tree;
ngx_rbtree_node_t sentinel;

static void
ngx_pagecount_rbtree_insert_value(ngx_rbtree_node_t *temp,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
    ngx_rbtree_node_t    **p;
// 红黑树的查找
    for ( ;; ) {

        if (node->key < temp->key) {

            p = &temp->left;

        } else if (node->key > temp->key) {

            p = &temp->right;

        } else { /* node->key == temp->key */

            return ;
        }

        if (*p == sentinel) {
            break;//到达了叶子结点
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}



#endif

//  ./sbin/nginx -c conf/nginx.conf
static char *ngx_http_count_module_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
#if ENABLE_RBTREE
	ngx_rbtree_init(&ngx_pv_tree, &sentinel, ngx_pagecount_rbtree_insert_value);
#endif
// 拿到配置文件
	ngx_http_core_loc_conf_t *corecf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	corecf->handler = ngx_http_count_module_handler;
// 请求一次就会执行到这个handler这个地方
	return NGX_CONF_OK;
}


#if ENABLE_RBTREE

//char content[1024];


void ngx_http_count_rbtree_iterator(ngx_rbtree_node_t *node, char *html) {

	char buffer[128] = {0};

	struct in_addr addr = {0};
	memcpy(&addr, &node->key, sizeof(struct in_addr));
	
	sprintf(buffer, "req from : %s, count: %d <br/>",
		inet_ntoa(addr), node->value);

	strcat(html, buffer);

}


static int ngx_encode_http_page_rb(char *html) {

	sprintf(html, "<h1>Source Insight </h1>");
	strcat(html, "<h2>");

	ngx_rbtree_traversal(&ngx_pv_tree, ngx_pv_tree.root, ngx_http_count_rbtree_iterator, html);

	strcat(html, "</h2>");

	return 0;
}

#else

#if 1
static int ngx_encode_http_page(char *html) {
	sprintf(html, "<h1>Source Insight </h1>");
	strcat(html, "<h2>");

	int i = 0;
	for (i = 0;i < 256;i ++) {
		if (pv_table[i].count != 0) {

			char str[INET_ADDRSTRLEN] = {0};
			char buffer[128] = {0};

sprintf(buffer, "req from : %s, count: %d <br/>",inet_ntop(AF_INET, &pv_table[i].addr, str, sizeof(str)), pv_table[i].count);
			strcat(html, buffer);
		}
	}
	
	strcat(html, "</h2>");

	return 0;
}
#endif


#endif

static ngx_int_t ngx_http_count_module_handler(ngx_http_request_t *r) {
#if 0
	u_char html[1024] = "<h1> bailang, Reese, feifei, qizhewoniuqukandahai, huihuisasa, magic, jianyuebujiandan, yuan </h1>";
	int len = sizeof(html);
#elif 1

	u_char html[1024] = {0};
	int len = sizeof(html);
	
#if ENABLE_RBTREE
	ngx_rbtree_node_t *node = NULL;
	ngx_rbtree_key_t key = 0;
#endif
    // 将请求中的ip地址取出来
	struct sockaddr_in *client_addr =  (struct sockaddr_in*)r->connection->sockaddr;
	//rbtree <key, value>  --> <addr, count>

	int idx = client_addr->sin_addr.s_addr >> 24;

	//rbtree --> (key, value)
	pv_table[idx].count ++;
	memcpy(&pv_table[idx].addr, &client_addr->sin_addr, sizeof(client_addr->sin_addr));

	
#if ENABLE_RBTREE
// 为什么nginx里面没有红黑树的查找
	key = (ngx_rbtree_key_t)client_addr->sin_addr.s_addr;
	ngx_log_error(NGX_LOG_EMERG, r->connection->log, ngx_errno, " ngx_http_count_module_handler --> %x\n", key);
	node = ngx_rbtree_search(&ngx_pv_tree, key);
	ngx_log_error(NGX_LOG_EMERG, r->connection->log, ngx_errno, " 11111 --> %x\n", key);
	
	if (node == &sentinel) {

		
		ngx_log_error(NGX_LOG_EMERG, r->connection->log, ngx_errno, " new node insert rbtree\n", key);
		node = ngx_pcalloc(r->pool, sizeof(ngx_rbtree_node_t));
		node->key = (ngx_rbtree_key_t)client_addr->sin_addr.s_addr;
		node->value = 1;

		ngx_rbtree_insert(&ngx_pv_tree, node);
	} else {
		node->value ++;

		ngx_log_error(NGX_LOG_EMERG, r->connection->log, ngx_errno, " node is exist, value:%d\n", node->value);
		
	}
	//memset(content, 0, 1024);
	ngx_encode_http_page_rb((char*)html);

	ngx_log_error(NGX_LOG_EMERG, r->connection->log, ngx_errno, " html: %s\n", html);
		
	//memcpy(html, content, len);
#else
	
	ngx_encode_http_page((char*)html);

#endif

#else

	

	struct sockaddr_in *client_addr =  (struct sockaddr_in*)r->connection->sockaddr;
	ngx_rbtree_node_t *node = ngx_pcalloc(r->pool,  sizeof(ngx_rbtree_node_t));
	
	memcpy(&node->key, &client_addr->sin_addr.s_addr, sizeof(node->key));
	

#endif
	//header
	r->headers_out.status = 200;
	ngx_str_set(&r->headers_out.content_type, "text/html");
	ngx_http_send_header(r);

	//body
	ngx_buf_t *b = ngx_pcalloc(r->pool,  sizeof(ngx_buf_t));

	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;

	b->pos = html;
	b->last = html+len;
	b->memory = 1;
	b->last_buf = 1;

	return ngx_http_output_filter(r, &out);
	
	
}

