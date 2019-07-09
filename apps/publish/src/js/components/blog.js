import React, { Component } from 'react';
import classnames from 'classnames';
import { PostPreview } from '/components/lib/post-preview';
import _ from 'lodash';
import { PathControl } from '/components/lib/path-control';
import { withRouter } from 'react-router';

const PC = withRouter(PathControl);

class Subscribe extends Component {
  constructor(props) {
    super(props);
  }

  render() {
    if (this.props.actionType === 'subscribe') {
      return (
        <p className="label-small-2 b pointer"
          onClick={this.props.subscribe}>
          Subscribe
        </p>
      );
    } else if (this.props.actionType === 'unsubscribe') {
      return (
        <p className="label-small-2 b pointer"
          onClick={this.props.unsubscribe}>
          Unsubscribe
        </p>
      );
    } else {
      return null;
    }
  }
}

export class Blog extends Component {
  constructor(props){
    super(props);

    this.state = {
      awaiting: false,
      postProps: [],
      blogTitle: '',
      blogHost: '',
      pathData: [],
      temporary: false,
      awaitingSubscribe: false,
      awaitingUnsubscribe: false,
    };

    this.subscribe = this.subscribe.bind(this);
    this.unsubscribe = this.unsubscribe.bind(this);
  }

  handleEvent(diff) {
    if (diff.data.total) {
      let blog = diff.data.total.data;
      this.setState({
        postProps: this.buildPosts(blog),
        blog: blog,
        blogTitle: blog.info.title,
        blogHost: blog.info.owner,
        awaiting: false,
        pathData: [
          { text: "Home", url: "/~publish/recent" },
          { text: blog.info.title, 
            url: `/~publish/${blog.info.owner}/${blog.info.filename}` }
        ],
      });

      this.props.setSpinner(false);
    }
  }

  handleError(err) {
  }

  componentDidUpdate(prevProps, prevState) {
    let ship = this.props.ship;
    let blogId = this.props.blogId;

    let blog = (ship == window.ship)
      ?  _.get(this.props, `pubs[${blogId}]`, false)
      :  _.get(this.props, `subs[${ship}][${blogId}]`, false);

    if (this.state.awaitingSubscribe) {
      if (blog) {
        this.setState({
          temporary: false,
          awaitingSubscribe: false,
        });

        this.props.setSpinner(false);
      }
    }
  }

  componentWillMount() {
    let ship = this.props.ship;
    let blogId = this.props.blogId;
    let blog = (ship == window.ship)
      ?  _.get(this.props, `pubs[${blogId}]`, false)
      :  _.get(this.props, `subs[${ship}][${blogId}]`, false);

    let temporary = (!(blog) && (ship != window.ship));

    if (temporary) {
      this.setState({
        awaiting: {
          ship: ship,
          blogId: blogId,
        },
        temporary: true,
      });

      this.props.setSpinner(true);

      this.props.api.bind(`/collection/${blogId}`, "PUT", ship, "write",
        this.handleEvent.bind(this),
        this.handleError.bind(this));
    }
  }

  buildPosts(blog){
    let pinProps = blog.order.pin.map((postId) => {
      let post = blog.posts[postId];
      return this.buildPostPreviewProps(post, blog, true);
    });

    let unpinProps = blog.order.unpin.map((postId) => {
      let post = blog.posts[postId];
      return this.buildPostPreviewProps(post, blog, false);
    });

    return pinProps.concat(unpinProps);
  }

  buildPostPreviewProps(post, blog, pinned){
    return {
      postTitle: post.post.info.title,
      postName:  post.post.info.filename,
      postBody: post.post.body,
      numComments: post.comments.length,
      collectionTitle: blog.info.title,
      collectionName:  blog.info.filename,
      author: post.post.info.creator,
      blogOwner: blog.info.owner,
      date: post.post.info["date-created"],
      pinned: pinned,
    }
  }

  buildData(){
    let blog = (this.props.ship == window.ship)
      ?  _.get(this.props, `pubs[${this.props.blogId}]`, false)
      :  _.get(this.props, `subs[${this.props.ship}][${this.props.blogId}]`, false);

    if (this.state.temporary) {
      return {
        blog: this.state.blog,
        postProps: this.state.postProps,
        blogTitle: this.state.blogTitle,
        blogHost: this.state.blogHost,
        pathData: this.state.pathData,
      };
    } else {
      return {
        blog: blog,
        postProps: this.buildPosts(blog),
        blogTitle: blog.info.title,
        blogHost: blog.info.owner,
        pathData: [
          { text: "Home", url: "/~publish/recent" },
          { text: blog.info.title, 
            url: `/~publish/${blog.info.owner}/${blog.info.filename}` }
        ],
      };
    }
  }

  subscribe() {
    let sub = {
      subscribe: {
        who: this.props.ship,
        coll: this.props.blogId,
      }
    }
    this.props.setSpinner(true);
    this.setState({awaitingSubscribe: true}, () => {
      this.props.api.action("write", "write-action", sub);
    });
  }

  unsubscribe() {
    let unsub = {
      unsubscribe: {
        who: this.props.ship,
        coll: this.props.blogId,
      }
    }
    this.props.api.action("write", "write-action", unsub);
    this.props.history.push("/~publish/recent");
  }

  render() {
    let data = this.buildData();

    let posts = data.postProps.map((post, key) => {
      return (
        <PostPreview
          post={post}
          key={key}
        />
      );
    });

    let contributors = `~${this.props.ship}`;

    if (this.state.awaiting) {
      return null;
    } else {
      let create = (this.props.ship === window.ship);

      let subscribers = 'None';
      let subNum = data.blog.subscribers.length

      if (subNum === 1) {
        subscribers = `~${data.blog.subscribers[0]}`;
      } else if (subNum === 2) {
        subscribers = `~${data.blog.subscribers[0]} and 1 other`;
      } else if (subNum > 2) {
        subscribers = `~${data.blog.subscribers[0]} and ${subNum-1} others`;
      }

      let foreign = _.get(this.props,
        `subs[${this.props.ship}][${this.props.blogId}]`, false);

      let actionType = false;
      if (this.state.temporary) {
        actionType = 'subscribe';
      } else if ((this.props.ship !== window.ship) && foreign) {
        actionType = 'unsubscribe';
      }

      return (
        <div>
          <PC pathData={data.pathData} create={create}/>
          <div className="absolute"
            style={{top:124, marginLeft: 16, marginRight: 16, marginTop: 32}}>
            <div className="flex-col">
              <h2>{data.blogTitle}</h2>
              <div className="flex" style={{marginTop: 22}}>
                <div style={{flexBasis: 160, marginRight:16}}>
                  <p className="gray-50 label-small-2 b">Host</p>
                  <p className="label-small-2">{data.blogHost}</p>
                </div>
                <div style={{flexBasis: 160, marginRight:16}}>
                  <p className="gray-50 label-small-2 b">Contributors</p>
                  <p className="label-small-2">{contributors}</p>
                </div>
                <div style={{flexBasis: 160, marginRight: 16}}>
                  <p className="gray-50 label-small-2 b">Subscribers</p>
                  <p className="label-small-2">{subscribers}</p>
                  <Subscribe actionType={actionType}
                    subscribe={this.subscribe}
                    unsubscribe={this.unsubscribe}
                  />
                </div>
              </div>
              <div className="flex flex-wrap" style={{marginTop: 48}}>
                {posts}
              </div>
            </div>
          </div>
        </div>
      );
    }
  }
}

