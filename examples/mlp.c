#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "kann.h"
#include "kann_extra/kann_data.h"

static kann_t *model_gen(int n_in, int n_out, int n_h_layers, int n_h_neurons, float h_dropout)
{
	int i;
	kad_node_t *t;
	t = kann_layer_input(n_in);
	for (i = 0; i < n_h_layers; ++i)
		t = kann_layer_dropout(kad_relu(kann_layer_linear(t, n_h_neurons)), h_dropout);
	return kann_layer_final(t, n_out, KANN_C_CEB);
}

int main(int argc, char *argv[])
{
	int i, j, c, n_h_neurons = 64, n_h_layers = 1, seed = 11;
	kann_data_t *in = 0;
	kann_mopt_t mo;
	kann_t *ann = 0;
	char *out_fn = 0, *in_fn = 0;
	float h_dropout = 0.0f;

	kann_mopt_init(&mo);
	mo.max_epoch = 50;
	while ((c = getopt(argc, argv, "n:l:s:r:m:B:o:i:d:R")) >= 0) {
		if (c == 'n') n_h_neurons = atoi(optarg);
		else if (c == 'l') n_h_layers = atoi(optarg);
		else if (c == 's') seed = atoi(optarg);
		else if (c == 'i') in_fn = optarg;
		else if (c == 'o') out_fn = optarg;
		else if (c == 'r') mo.lr = atof(optarg);
		else if (c == 'm') mo.max_epoch = atoi(optarg);
		else if (c == 'B') mo.max_mbs = atoi(optarg);
		else if (c == 'd') h_dropout = atof(optarg);
		else if (c == 'R') mo.batch_algo = KANN_MB_iRprop;
	}
	if (argc - optind < 1) {
		FILE *fp = stdout;
		fprintf(fp, "Usage: mlp [options] <in.knd> [out.knd]\n");
		fprintf(fp, "Options:\n");
		fprintf(fp, "  Model construction:\n");
		fprintf(fp, "    -i FILE     read trained model from FILE []\n");
		fprintf(fp, "    -o FILE     save trained model to FILE []\n");
		fprintf(fp, "    -s INT      random seed [%d]\n", seed);
		fprintf(fp, "    -l INT      number of hidden layers [%d]\n", n_h_layers);
		fprintf(fp, "    -n INT      number of hidden neurons per layer [%d]\n", n_h_neurons);
		fprintf(fp, "  Model training:\n");
		fprintf(fp, "    -r FLOAT    learning rate [%g]\n", mo.lr);
		fprintf(fp, "    -d FLOAT    dropout at the hidden layer(s) [%g]\n", h_dropout);
		fprintf(fp, "    -m INT      max number of epochs [%d]\n", mo.max_epoch);
		fprintf(fp, "    -B INT      mini-batch size [%d]\n", mo.max_mbs);
		fprintf(fp, "    -R          use iRprop- after a batch\n");
		return 1;
	}
	if (argc - optind == 1 && in_fn == 0) {
		fprintf(stderr, "ERROR: please specify a trained model with option '-i'.\n");
		return 1;
	}

	kad_trap_fe();
	kann_srand(seed);
	in = kann_data_read(argv[optind]);
	if (in_fn) {
		ann = kann_read(in_fn);
		assert(kann_n_in(ann) == in->n_col);
	}

	if (optind+1 < argc) { // train
		kann_data_t *out;
		out = kann_data_read(argv[optind+1]);
		assert(in->n_row == out->n_row);
		if (ann) assert(kann_n_out(ann) == out->n_col);
		else ann = model_gen(in->n_col, out->n_col, n_h_layers, n_h_neurons, h_dropout);
		kann_fnn_train(&mo, ann, in->n_row, in->x, out->x);
		if (out_fn) kann_write(out_fn, ann);
		kann_data_free(out);
	} else { // apply
		int n_out;
		kann_set_hyper(ann, KANN_H_DROPOUT, 0.0f);
		n_out = kann_n_out(ann);
		for (i = 0; i < in->n_row; ++i) {
			const float *y;
			y = kann_apply1(ann, in->x[i]);
			if (in->rname) printf("%s\t", in->rname[i]);
			for (j = 0; j < n_out; ++j) {
				if (j) putchar('\t');
				printf("%.3g", y[j] + 1.0f - 1.0f);
			}
			putchar('\n');
		}
	}

	kann_delete(ann);
	kann_data_free(in);
	return 0;
}
