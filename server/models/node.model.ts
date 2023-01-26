class MeshNode {
    id: string;
    name: string;
    status: string;
    timer: any;

    constructor(id: string, name: string, status: string) {
        this.id = id;
        this.name = name;
        this.status = status;
    }
    toString() {
        return JSON.stringify(this);
    }
    fromJson(json: string) {
        var obj = JSON.parse(json);
        this.id = obj.id;
        this.name = obj.name;
        this.status = obj.status;
    }
}

class NodeList {
    static nodes: MeshNode[] = [];
    static addNode(node: MeshNode) {
        if (this.nodes.find(n => n.id == node.id)) {
            this.removeNode(node.id);
        }
        this.nodes.push(node);
    }
    static clear() {
        this.nodes = [];
    }
    static removeNode(id: string) {
        var index = this.nodes.findIndex(n => n.id == id);
        if (index > -1) {
            this.nodes.splice(index, 1);
            return true;
        }
        return false;
    }
    static getNode(id: string) {
        return this.nodes.find(n => n.id == id);
    }
    static toString() {
        // Generate JSON for each node but without the timer
        var nodes = this.nodes.map(n => {
            var node = new MeshNode(n.id, n.name, n.status);
            return node;
        });
        // Order by name
        nodes.sort((a, b) => {
            if (a.name < b.name) {
                return -1;
            }
            if (a.name > b.name) {
                return 1;
            }
            return 0;
        });
        return JSON.stringify(nodes);
    }
    static fromJson(json: string) {
        var obj = JSON.parse(json);
        this.nodes = obj.nodes;
    }
}

export { MeshNode, NodeList };